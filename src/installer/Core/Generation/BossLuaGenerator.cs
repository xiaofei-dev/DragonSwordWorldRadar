using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    public static class BossLuaGenerator
    {
        private const string FieldBossIconAsset =
            "/Script/Paper2D.PaperSprite'/Game/Art/UI/InGame/Common/Mark/" +
            "Icon_Mark_FieldBoss_Sprite.Icon_Mark_FieldBoss_Sprite'";

        private sealed class BossDefinition
        {
            public uint Id;
            public int MapId;
            public uint SwitchWeekId;

            public BossDefinition(uint id, int mapId, uint switchWeekId)
            {
                Id = id;
                MapId = mapId;
                SwitchWeekId = switchWeekId;
            }
        }

        private static readonly BossDefinition[] BossDefinitions =
        {
            new BossDefinition(9000010, 100, 90000),
            new BossDefinition(9000011, 100, 90001),
            new BossDefinition(9000005, 100, 90002),
            new BossDefinition(9000012, 100, 90003),
            new BossDefinition(9000007, 100, 90004),
            new BossDefinition(9000019, 100, 90005),
            new BossDefinition(9000022, 200, 90006),
            new BossDefinition(9000023, 200, 90007),
            new BossDefinition(9000025, 200, 90008),
        };

        public static int Generate(
            string sectionMonsterXmlPath,
            string outputPath)
        {
            XmlDocument monsterDocument = Load(sectionMonsterXmlPath);

            Dictionary<uint, XmlElement> monsters =
                ReadBossMonsters(monsterDocument);

            List<string> lines = new List<string>
            {
                "-- Generated locally by DragonSwordWorldRadar.",
                "-- World-boss definitions are independent from treasure save-state data.",
                "return {"
            };

            foreach (BossDefinition configured in BossDefinitions)
            {
                XmlElement monster;
                if (!monsters.TryGetValue(configured.Id, out monster))
                {
                    throw new InvalidDataException(
                        "World-boss monster data is incomplete for ID " +
                        configured.Id.ToString(CultureInfo.InvariantCulture) + ".");
                }

                string x = Required(monster, "PosX");
                string y = Required(monster, "PosY");
                string z = Required(monster, "PosZ");
                string uid = Required(monster, "UID");
                string uidName = Required(monster, "UIDName");
                string switchWeekId = configured.SwitchWeekId.ToString(
                    CultureInfo.InvariantCulture);
                string respawnCycleId = Optional(monster, "RespawnCycleID") ?? "0";
                string serverDeathCheck = Optional(monster, "ServerDeathCheck") ?? "False";

                ValidateNumber(x, "PosX");
                ValidateNumber(y, "PosY");
                ValidateNumber(z, "PosZ");
                ValidateUnsigned(uid, "UID");
                ValidateUnsigned(switchWeekId, "SwitchWeekID");
                ValidateUnsigned(respawnCycleId, "RespawnCycleID");
                if (!String.Equals(
                    serverDeathCheck,
                    "True",
                    StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException(
                        "World-boss ServerDeathCheck must be True for ID " +
                        configured.Id.ToString(CultureInfo.InvariantCulture) + ".");
                }

                StringBuilder line = new StringBuilder();
                line.Append("    { boss_id = ");
                line.Append(configured.Id.ToString(CultureInfo.InvariantCulture));
                line.Append(", map_id = ");
                line.Append(configured.MapId.ToString(CultureInfo.InvariantCulture));
                line.Append(", x = ");
                line.Append(x);
                line.Append(", y = ");
                line.Append(y);
                line.Append(", z = ");
                line.Append(z);
                line.Append(", uid = \"");
                line.Append(EscapeLuaString(uid));
                line.Append("\", uid_name = \"");
                line.Append(EscapeLuaString(uidName));
                line.Append("\", switch_week_id = ");
                line.Append(switchWeekId);
                line.Append(", respawn_cycle_id = ");
                line.Append(respawnCycleId);
                line.Append(", server_death_check = true");
                line.Append(", icon_asset = \"");
                line.Append(EscapeLuaString(FieldBossIconAsset));
                line.Append("\" },");
                lines.Add(line.ToString());
            }
            lines.Add("}");

            Directory.CreateDirectory(Path.GetDirectoryName(outputPath));
            File.WriteAllLines(outputPath, lines, new UTF8Encoding(false));
            return BossDefinitions.Length;
        }

        private static XmlDocument Load(string path)
        {
            XmlDocument document = new XmlDocument { XmlResolver = null };
            document.Load(path);
            return document;
        }


        private static Dictionary<uint, XmlElement> ReadBossMonsters(
            XmlDocument document)
        {
            Dictionary<uint, XmlElement> result =
                new Dictionary<uint, XmlElement>();
            // The generated XML element name has changed between game-data
            // revisions. Match by the required CID attribute instead of
            // coupling the provider to one wrapper/local-name.
            XmlNodeList nodes = document.SelectNodes("//*");
            if (nodes == null)
            {
                return result;
            }
            foreach (XmlNode node in nodes)
            {
                XmlElement element = node as XmlElement;
                uint cid;
                if (element != null
                    && uint.TryParse(
                        Optional(element, "CID"),
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out cid)
                    && IsBossId(cid))
                {
                    if (result.ContainsKey(cid))
                    {
                        throw new InvalidDataException(
                            "Multiple world-boss monster rows were found for ID " +
                            cid.ToString(CultureInfo.InvariantCulture) + ".");
                    }
                    result[cid] = element;
                }
            }
            return result;
        }


        private static bool IsBossId(uint id)
        {
            foreach (BossDefinition definition in BossDefinitions)
            {
                if (definition.Id == id)
                {
                    return true;
                }
            }
            return false;
        }

        private static string Required(XmlElement element, string name)
        {
            string value = Optional(element, name);
            if (!String.IsNullOrWhiteSpace(value))
            {
                return value.Trim();
            }
            throw new InvalidDataException(
                "World-boss data is missing field '" + name + "'.");
        }

        private static string Optional(XmlElement element, string name)
        {
            foreach (XmlAttribute attribute in element.Attributes)
            {
                if (String.Equals(
                    attribute.LocalName,
                    name,
                    StringComparison.OrdinalIgnoreCase))
                {
                    return attribute.Value;
                }
            }
            return null;
        }

        private static void ValidateUnsigned(string value, string field)
        {
            ulong ignored;
            if (!ulong.TryParse(
                value,
                NumberStyles.None,
                CultureInfo.InvariantCulture,
                out ignored))
            {
                throw new InvalidDataException(field + " contains an invalid value.");
            }
        }

        private static void ValidateNumber(string value, string field)
        {
            double parsed;
            if (!double.TryParse(
                    value,
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out parsed)
                || Double.IsNaN(parsed)
                || Double.IsInfinity(parsed))
            {
                throw new InvalidDataException(field + " contains an invalid value.");
            }
        }

        private static string EscapeLuaString(string value)
        {
            return (value ?? String.Empty)
                .Replace("\\", "\\\\")
                .Replace("\"", "\\\"")
                .Replace("\r", "\\r")
                .Replace("\n", "\\n");
        }
    }
}
