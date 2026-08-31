using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    public static class AdditionalMiniGameLuaGenerator
    {
        private static readonly Regex StartPattern = new Regex(
            @"^MiniGame_(?<type>Mole|Wave)_(?<id>12(?:00[1-9]|0[1-3][0-9]|040)|130(?:0[1-9]|10))_(?<role>NPC_Start|Teleport_Start)$",
            RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

        private sealed class Point
        {
            public int Id;
            public string Type;
            public int MapId;
            public double X;
            public double Y;
            public double Z;
            public bool HasZ;
            public string Name;
            public string Source;
            public string Role;
            public int Priority;
        }

        public static void Append(string[] xmlPaths, string outputPath)
        {
            Dictionary<int, Point> points = new Dictionary<int, Point>();
            foreach (string path in xmlPaths)
            {
                if (!File.Exists(path)) continue;
                XmlDocument document = new XmlDocument();
                document.XmlResolver = null;
                document.Load(path);
                XmlNodeList nodes = document.SelectNodes("//*");
                if (nodes == null) continue;
                foreach (XmlNode node in nodes)
                {
                    XmlElement element = node as XmlElement;
                    if (element == null) continue;
                    string name = Read(element, "Name", "UIDName", "NodeName", "ActorName");
                    Match match = StartPattern.Match(name ?? "");
                    if (!match.Success) continue;
                    int id = Int32.Parse(match.Groups["id"].Value, CultureInfo.InvariantCulture);
                    Point point = new Point();
                    point.Id = id;
                    point.Type = match.Groups["type"].Value.Equals("Wave", StringComparison.OrdinalIgnoreCase) ? "wave" : "mole";
                    point.Name = name;
                    point.Role = match.Groups["role"].Value;
                    point.Priority = point.Role.Equals("NPC_Start", StringComparison.OrdinalIgnoreCase) ? 2 : 1;
                    point.Source = Path.GetFileName(path).Replace('\\', '/');
                    if (!TryInt(element, out point.MapId, "MapGroupID", "MapID", "WorldMapID")
                        || !TryDouble(element, out point.X, "PosX", "PositionX", "LocationX", "X")
                        || !TryDouble(element, out point.Y, "PosY", "PositionY", "LocationY", "Y"))
                    {
                        continue;
                    }
                    point.HasZ = TryDouble(element, out point.Z, "PosZ", "PositionZ", "LocationZ", "Z");
                    Point existing;
                    if (points.TryGetValue(id, out existing)
                        && existing.Priority == point.Priority
                        && (existing.MapId != point.MapId || existing.X != point.X || existing.Y != point.Y))
                    {
                        throw new InvalidDataException("Conflicting Teleport_Start positions for MiniGame " + id.ToString(CultureInfo.InvariantCulture) + ".");
                    }
                    if (existing == null || point.Priority > existing.Priority) points[id] = point;
                }
            }
            for (int id = 12001; id <= 12040; id++) Require(points, id);
            for (int id = 13001; id <= 13010; id++) Require(points, id);
            if (points.Count != 50) throw new InvalidDataException("Expected exactly 50 Mole/Wave start points.");

            string text = File.ReadAllText(outputPath);
            int closing = text.LastIndexOf('}');
            if (closing < 0) throw new InvalidDataException("Generated Fly catalog has no closing table delimiter.");
            StringBuilder extra = new StringBuilder();
            foreach (Point point in Sorted(points))
            {
                extra.Append("    { mini_game_id = ").Append(point.Id.ToString(CultureInfo.InvariantCulture));
                extra.Append(", mask_bit = -1, mini_game_type = \"").Append(point.Type).Append("\"");
                extra.Append(", map_id = ").Append(point.MapId.ToString(CultureInfo.InvariantCulture));
                extra.Append(", x = ").Append(point.X.ToString("R", CultureInfo.InvariantCulture));
                extra.Append(", y = ").Append(point.Y.ToString("R", CultureInfo.InvariantCulture));
                if (point.HasZ) extra.Append(", z = ").Append(point.Z.ToString("R", CultureInfo.InvariantCulture));
                extra.Append(", has_z = ").Append(point.HasZ ? "true" : "false");
                extra.Append(", uid_name = \"").Append(Escape(point.Name)).Append("\"");
                extra.Append(", source_entry = \"").Append(Escape(point.Source)).Append("\"");
                extra.Append(", anchor_score = 1300, anchor_evidence = \"explicit ").Append(point.Role).Append("\"");
                extra.Append(", position_role = \"").Append(point.Role).Append("\", notice_title = \"\", notice_description = \"\" },\r\n");
            }
            text = text.Substring(0, closing) + extra + text.Substring(closing);
            File.WriteAllText(outputPath, text, new UTF8Encoding(false));
        }

        private static IEnumerable<Point> Sorted(Dictionary<int, Point> points)
        {
            List<int> ids = new List<int>(points.Keys);
            ids.Sort();
            foreach (int id in ids) yield return points[id];
        }

        private static void Require(Dictionary<int, Point> points, int id)
        {
            if (!points.ContainsKey(id)) throw new InvalidDataException("Missing explicit NPC_Start/Teleport_Start for MiniGame " + id.ToString(CultureInfo.InvariantCulture) + ".");
        }

        private static string Read(XmlElement element, params string[] names)
        {
            foreach (string name in names)
            {
                if (element.HasAttribute(name)) return element.GetAttribute(name).Trim();
                foreach (XmlAttribute attribute in element.Attributes)
                {
                    if (attribute.LocalName.Equals(name, StringComparison.OrdinalIgnoreCase))
                    {
                        return attribute.Value.Trim();
                    }
                }
                XmlNode node = element.SelectSingleNode(".//*[local-name()='" + name + "']");
                if (node != null && !String.IsNullOrWhiteSpace(node.InnerText)) return node.InnerText.Trim();
            }
            return null;
        }

        private static bool TryInt(XmlElement element, out int value, params string[] names)
        {
            return Int32.TryParse(Read(element, names), NumberStyles.Integer, CultureInfo.InvariantCulture, out value) && value > 0;
        }

        private static bool TryDouble(XmlElement element, out double value, params string[] names)
        {
            return Double.TryParse(Read(element, names), NumberStyles.Float, CultureInfo.InvariantCulture, out value)
                && !Double.IsNaN(value) && !Double.IsInfinity(value);
        }

        private static string Escape(string value)
        {
            return (value ?? "").Replace("\\", "\\\\").Replace("\"", "\\\"");
        }
    }
}
