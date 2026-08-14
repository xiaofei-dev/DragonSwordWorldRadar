using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    public static class MoleLuaGenerator
    {
        private const int FirstMoleId = 11001;
        private const int LastMoleId = 11034;
        private const int ExpectedOrdinaryFlightCount = 33;
        private const string OrdinaryFlightNoticeTitle = "109208";
        private const string OrdinaryFlightNoticeDescription = "109202";
        private const string TeleportStartRole = "Teleport_Start";
        private const string NpcStartRole = "NPC_Start";
        private const string MoleLinkedRole = "Fly_Linked";

        private static readonly string[][] CoordinateTriplets =
        {
            new[] { "PosX", "PosY", "PosZ" },
            new[] { "PositionX", "PositionY", "PositionZ" },
            new[] { "LocationX", "LocationY", "LocationZ" },
            new[] { "WorldX", "WorldY", "WorldZ" },
            new[] { "ActorPosX", "ActorPosY", "ActorPosZ" },
            new[] { "X", "Y", "Z" },
        };

        private static readonly string[] VectorFields =
        {
            "Position",
            "Pos",
            "Location",
            "WorldLocation",
            "ActorLocation",
            "Transform",
        };

        private static readonly Regex MoleNamePattern = new Regex(
            @"(?i)(?:MiniGame[_\-]?)?Fly[_\-]?(110(?:0[1-9]|[12][0-9]|3[0-4]))",
            RegexOptions.CultureInvariant);

        private static readonly Regex VectorPattern = new Regex(
            @"(?i)X\s*=\s*(?<x>[+\-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+\-]?\d+)?)" +
            @"[^\r\n]*?Y\s*=\s*(?<y>[+\-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+\-]?\d+)?)" +
            @"(?:[^\r\n]*?Z\s*=\s*(?<z>[+\-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+\-]?\d+)?))?",
            RegexOptions.CultureInvariant);

        private static readonly Regex MapNamePattern = new Regex(
            @"(?i)(?:Map|Level|Section)[_\-/]?(?<id>\d{3,6})",
            RegexOptions.CultureInvariant);

        private static readonly Regex WorldNamePattern = new Regex(
            @"(?i)World[_\-/]?(?<id>\d{1,2})(?:\D|$)",
            RegexOptions.CultureInvariant);

        private sealed class MoleCandidate
        {
            public int Id;
            public int MapId;
            public double X;
            public double Y;
            public double Z;
            public bool HasZ;
            public string UidName;
            public string PositionName;
            public string PositionRole;
            public string SourceEntry;
            public int Score;
            public int AnchorScore;
            public string AnchorEvidence;
            public string Source;
        }

        private sealed class MoleMiniGameRule
        {
            public int Id;
            public string NoticeTitle;
            public string NoticeDescription;
            public string DataLayer;
            public string Source;
        }

        public static int Generate(
            string miniGameXmlPath,
            string outputPath)
        {
            return Generate(new[] { miniGameXmlPath }, outputPath);
        }

        public static int Generate(
            string[] xmlPaths,
            string outputPath)
        {
            if (xmlPaths == null || xmlPaths.Length == 0)
            {
                throw new ArgumentException(
                    "At least one install-extracted XML source is required.",
                    "xmlPaths");
            }

            Dictionary<int, List<MoleCandidate>> candidates =
                new Dictionary<int, List<MoleCandidate>>();
            Dictionary<int, MoleMiniGameRule> miniGameRules =
                new Dictionary<int, MoleMiniGameRule>();
            int parsedFiles = 0;
            foreach (string xmlPath in xmlPaths
                .Where(path => !String.IsNullOrWhiteSpace(path))
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase))
            {
                if (!File.Exists(xmlPath))
                {
                    continue;
                }

                XmlDocument document = new XmlDocument();
                document.XmlResolver = null;
                document.Load(xmlPath);
                parsedFiles++;

                XmlNodeList nodes = document.SelectNodes("//*");
                if (nodes == null)
                {
                    continue;
                }

                string sourceEntry = NormalizeSourceEntry(xmlPath);
                foreach (XmlNode node in nodes)
                {
                    XmlElement element = node as XmlElement;
                    if (element == null)
                    {
                        continue;
                    }

                    MoleMiniGameRule miniGameRule;
                    if (TryCreateMiniGameRule(
                        element,
                        sourceEntry,
                        out miniGameRule))
                    {
                        MoleMiniGameRule existingRule;
                        if (!miniGameRules.TryGetValue(
                                miniGameRule.Id,
                                out existingRule))
                        {
                            miniGameRules[miniGameRule.Id] = miniGameRule;
                        }
                        else if (!SameMiniGameRule(
                            miniGameRule,
                            existingRule))
                        {
                            throw new InvalidDataException(
                                "Conflicting Fly gameplay classification for ID " +
                                miniGameRule.Id.ToString(
                                    CultureInfo.InvariantCulture) +
                                ": " + DescribeMiniGameRule(existingRule) +
                                " versus " + DescribeMiniGameRule(miniGameRule) + ".");
                        }
                    }

                    MoleCandidate candidate;
                    if (!TryCreateCandidate(
                        element,
                        sourceEntry,
                        out candidate))
                    {
                        continue;
                    }

                    List<MoleCandidate> bucket;
                    if (!candidates.TryGetValue(candidate.Id, out bucket))
                    {
                        bucket = new List<MoleCandidate>();
                        candidates[candidate.Id] = bucket;
                    }
                    bucket.Add(candidate);
                }
            }

            if (parsedFiles == 0)
            {
                throw new InvalidDataException(
                    "No install-extracted Fly XML source could be parsed.");
            }

            WriteCandidateReport(candidates, outputPath);

            List<int> missingRules = new List<int>();
            for (int id = FirstMoleId; id <= LastMoleId; id++)
            {
                if (!miniGameRules.ContainsKey(id))
                {
                    missingRules.Add(id);
                }
            }
            if (missingRules.Count != 0)
            {
                throw new InvalidDataException(
                    "MiniGameData classification is incomplete. Missing Fly IDs: " +
                    String.Join(",", missingRules.Select(value => value.ToString(
                        CultureInfo.InvariantCulture)).ToArray()) + ".");
            }

            List<int> eligibleIds = miniGameRules.Values
                .Where(IsOrdinaryFlightMiniGame)
                .Where(value => value.Id != 11024)
                .Select(value => value.Id)
                .Distinct()
                .OrderBy(value => value)
                .ToList();
            if (eligibleIds.Count != ExpectedOrdinaryFlightCount)
            {
                throw new InvalidDataException(
                    "The Fly mini-game set is incomplete or changed. " +
                    "Expected " + ExpectedOrdinaryFlightCount.ToString(
                        CultureInfo.InvariantCulture) +
                    " records with NoticeTitle=" + OrdinaryFlightNoticeTitle +
                    " and NoticeDescription=" +
                    OrdinaryFlightNoticeDescription +
                    "; found " + eligibleIds.Count.ToString(
                        CultureInfo.InvariantCulture) +
                    " (IDs=" + String.Join(",", eligibleIds.Select(
                        value => value.ToString(
                            CultureInfo.InvariantCulture)).ToArray()) + ").");
            }

            Dictionary<int, MoleCandidate> selected =
                new Dictionary<int, MoleCandidate>();
            List<int> missing = new List<int>();
            foreach (int id in eligibleIds)
            {
                List<MoleCandidate> bucket;
                if (!candidates.TryGetValue(id, out bucket)
                    || bucket.Count == 0)
                {
                    missing.Add(id);
                    continue;
                }

                List<MoleCandidate> ranked = bucket
                    .Where(value => GetPositionPriority(value) > 0)
                    .OrderByDescending(value => GetPositionPriority(value))
                    .ThenByDescending(value => value.AnchorScore)
                    .ThenByDescending(value => value.Score)
                    .ToList();
                if (ranked.Count == 0)
                {
                    throw new InvalidDataException(
                        "No explicit Fly start position was found for ID " +
                        id.ToString(CultureInfo.InvariantCulture) +
                        ". See data/generated/mole-anchor-candidates.tsv.");
                }

                int bestPriority = GetPositionPriority(ranked[0]);
                int bestAnchor = ranked[0].AnchorScore;
                int bestScore = ranked[0].Score;
                List<MoleCandidate> finalists = ranked
                    .Where(value => GetPositionPriority(value) == bestPriority
                        && value.AnchorScore == bestAnchor
                        && value.Score == bestScore)
                    .ToList();
                MoleCandidate winner = finalists[0];
                foreach (MoleCandidate candidate in finalists)
                {
                    if (!SameLocation(candidate, winner))
                    {
                        throw new InvalidDataException(
                            "Fly start position is still ambiguous for ID " +
                            id.ToString(CultureInfo.InvariantCulture) +
                            ": " + DescribeCandidate(winner) +
                            " versus " + DescribeCandidate(candidate) +
                            ". See data/generated/mole-anchor-candidates.tsv.");
                    }
                }
                selected[id] = winner;
            }
            if (missing.Count != 0)
            {
                throw new InvalidDataException(
                    "The Fly catalog is incomplete. Missing IDs: " +
                    String.Join(",", missing.Select(value => value.ToString(
                        CultureInfo.InvariantCulture)).ToArray()) + ".");
            }
            if (selected.Count != ExpectedOrdinaryFlightCount)
            {
                throw new InvalidDataException(
                    "The Fly catalog must contain exactly " +
                    ExpectedOrdinaryFlightCount.ToString(
                        CultureInfo.InvariantCulture) +
                    " validated start positions.");
            }

            List<string> lines = new List<string>();
            lines.Add("-- Generated locally by DragonSwordWorldRadar from install-extracted game XML.");
            lines.Add("-- Only MiniGame_Fly_11001-11034 are emitted; their start points are independent from treasure reward coordinates.");
            lines.Add("-- Position priority is NPC_Start, then Teleport_Start, then another Fly-linked position.");
            lines.Add("return {");
            for (int outputIndex = 0; outputIndex < eligibleIds.Count; outputIndex++)
            {
                int id = eligibleIds[outputIndex];
                MoleCandidate candidate = selected[id];
                StringBuilder line = new StringBuilder();
                line.Append("    { mini_game_id = ");
                line.Append(id.ToString(CultureInfo.InvariantCulture));
                line.Append(", mask_bit = ");
                line.Append(outputIndex.ToString(
                    CultureInfo.InvariantCulture));
                line.Append(", map_id = ");
                line.Append(candidate.MapId.ToString(
                    CultureInfo.InvariantCulture));
                line.Append(", x = ");
                line.Append(FormatNumber(candidate.X));
                line.Append(", y = ");
                line.Append(FormatNumber(candidate.Y));
                if (candidate.HasZ)
                {
                    line.Append(", z = ");
                    line.Append(FormatNumber(candidate.Z));
                }
                line.Append(", has_z = ");
                line.Append(candidate.HasZ ? "true" : "false");
                line.Append(", uid_name = \"");
                line.Append(EscapeLuaString(candidate.UidName));
                line.Append("\", source_entry = \"");
                line.Append(EscapeLuaString(candidate.SourceEntry));
                line.Append("\", anchor_score = ");
                line.Append(candidate.AnchorScore.ToString(CultureInfo.InvariantCulture));
                line.Append(", anchor_evidence = \"");
                line.Append(EscapeLuaString(candidate.AnchorEvidence));
                line.Append("\", position_role = \"");
                line.Append(EscapeLuaString(candidate.PositionRole));
                line.Append("\", notice_title = \"");
                line.Append(OrdinaryFlightNoticeTitle);
                line.Append("\", notice_description = \"");
                line.Append(OrdinaryFlightNoticeDescription);
                line.Append("\" },");
                lines.Add(line.ToString());
            }
            lines.Add("}");

            string directory = Path.GetDirectoryName(outputPath);
            if (!String.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }
            File.WriteAllLines(
                outputPath,
                lines,
                new UTF8Encoding(false));
            return selected.Count;
        }

        private static bool TryCreateMiniGameRule(
            XmlElement element,
            string sourceEntry,
            out MoleMiniGameRule rule)
        {
            rule = null;
            Dictionary<string, List<string>> fields =
                new Dictionary<string, List<string>>(
                    StringComparer.OrdinalIgnoreCase);
            CollectFields(element, fields, 0, 2);
            AddField(fields, "__ElementName", element.LocalName);

            int id;
            string uidName;
            int identityScore;
            if (!TryReadMoleIdentity(
                element,
                fields,
                out id,
                out uidName,
                out identityScore))
            {
                return false;
            }

            string noticeTitle = First(fields, "MiniGameNoticeTitle");
            string noticeDescription = First(fields, "MiniGameNoticeDescription");
            if (String.IsNullOrWhiteSpace(noticeTitle)
                || String.IsNullOrWhiteSpace(noticeDescription))
            {
                return false;
            }

            rule = new MoleMiniGameRule();
            rule.Id = id;
            rule.NoticeTitle = noticeTitle.Trim();
            rule.NoticeDescription = noticeDescription.Trim();
            rule.DataLayer = First(fields, "DataLayer", "Name", "UIDName");
            rule.Source = sourceEntry + ":" + BuildElementPath(element);
            return true;
        }

        private static bool SameMiniGameRule(
            MoleMiniGameRule left,
            MoleMiniGameRule right)
        {
            return left != null
                && right != null
                && left.Id == right.Id
                && String.Equals(
                    left.NoticeTitle,
                    right.NoticeTitle,
                    StringComparison.OrdinalIgnoreCase)
                && String.Equals(
                    left.NoticeDescription,
                    right.NoticeDescription,
                    StringComparison.OrdinalIgnoreCase);
        }

        private static string DescribeMiniGameRule(
            MoleMiniGameRule rule)
        {
            if (rule == null)
            {
                return "<null>";
            }
            return "title=" + rule.NoticeTitle +
                ", description=" + rule.NoticeDescription +
                ", source=" + rule.Source;
        }

        private static bool IsOrdinaryFlightMiniGame(
            MoleMiniGameRule rule)
        {
            return rule != null
                && String.Equals(
                    rule.NoticeTitle,
                    OrdinaryFlightNoticeTitle,
                    StringComparison.OrdinalIgnoreCase)
                && String.Equals(
                    rule.NoticeDescription,
                    OrdinaryFlightNoticeDescription,
                    StringComparison.OrdinalIgnoreCase);
        }

        private static int GetPositionPriority(MoleCandidate candidate)
        {
            if (candidate == null)
            {
                return 0;
            }
            if (String.Equals(
                candidate.PositionRole,
                NpcStartRole,
                StringComparison.OrdinalIgnoreCase))
            {
                return 300;
            }
            if (String.Equals(
                candidate.PositionRole,
                TeleportStartRole,
                StringComparison.OrdinalIgnoreCase))
            {
                return 200;
            }
            if (String.Equals(
                candidate.PositionRole,
                MoleLinkedRole,
                StringComparison.OrdinalIgnoreCase))
            {
                return 100;
            }
            return 0;
        }

        private static string ReadPositionRole(
            int id,
            string positionName)
        {
            if (String.IsNullOrWhiteSpace(positionName))
            {
                return null;
            }
            string expectedPrefix = "MiniGame_Fly_" +
                id.ToString(CultureInfo.InvariantCulture);
            if (positionName.StartsWith(
                expectedPrefix + "_" + TeleportStartRole,
                StringComparison.OrdinalIgnoreCase))
            {
                return TeleportStartRole;
            }
            if (positionName.StartsWith(
                expectedPrefix + "_" + NpcStartRole,
                StringComparison.OrdinalIgnoreCase))
            {
                return NpcStartRole;
            }
            if (String.Equals(
                    positionName,
                    expectedPrefix,
                    StringComparison.OrdinalIgnoreCase)
                || positionName.StartsWith(
                    expectedPrefix + "_",
                    StringComparison.OrdinalIgnoreCase))
            {
                return MoleLinkedRole;
            }
            return null;
        }

        private static bool TryCreateCandidate(
            XmlElement element,
            string sourceEntry,
            out MoleCandidate candidate)
        {
            candidate = null;
            Dictionary<string, List<string>> fields =
                new Dictionary<string, List<string>>(
                    StringComparer.OrdinalIgnoreCase);
            CollectFields(element, fields, 0, 2);
            AddField(fields, "__ElementName", element.LocalName);
            Dictionary<string, List<string>> directFields =
                new Dictionary<string, List<string>>(
                    StringComparer.OrdinalIgnoreCase);
            CollectDirectFields(element, directFields);

            int id;
            string uidName;
            int identityScore;
            if (!TryReadMoleIdentity(
                element,
                fields,
                out id,
                out uidName,
                out identityScore))
            {
                return false;
            }

            double x;
            double y;
            double z;
            bool hasZ;
            int coordinateScore;
            if (!TryReadCoordinates(
                fields,
                out x,
                out y,
                out z,
                out hasZ,
                out coordinateScore))
            {
                return false;
            }

            int mapId;
            int mapScore;
            if (!TryReadMapId(fields, out mapId, out mapScore))
            {
                return false;
            }

            if (!IsFinite(x)
                || !IsFinite(y)
                || (Math.Abs(x) < 0.001 && Math.Abs(y) < 0.001)
                || (hasZ && !IsFinite(z)))
            {
                return false;
            }

            candidate = new MoleCandidate();
            candidate.Id = id;
            candidate.MapId = mapId;
            candidate.X = x;
            candidate.Y = y;
            candidate.Z = z;
            candidate.HasZ = hasZ;
            string positionName = First(
                directFields,
                "Name",
                "UIDName",
                "NodeName",
                "ActorName");
            string positionRole = ReadPositionRole(id, positionName);
            candidate.UidName = !String.IsNullOrWhiteSpace(positionName)
                ? positionName.Trim()
                : (String.IsNullOrWhiteSpace(uidName)
                    ? "MiniGame_Fly_" + id.ToString(
                        CultureInfo.InvariantCulture)
                    : uidName.Trim());
            candidate.PositionName = positionName;
            candidate.PositionRole = positionRole;
            candidate.SourceEntry = sourceEntry;
            candidate.Score = identityScore + coordinateScore + mapScore;
            string anchorEvidence;
            candidate.AnchorScore = ReadAnchorScore(
                element,
                directFields,
                id,
                out anchorEvidence);
            candidate.AnchorEvidence = anchorEvidence;
            if (element.LocalName.IndexOf(
                    "Fly",
                    StringComparison.OrdinalIgnoreCase) >= 0)
            {
                candidate.Score += 15;
            }
            if (element.LocalName.IndexOf(
                    "MiniGame",
                    StringComparison.OrdinalIgnoreCase) >= 0)
            {
                candidate.Score += 8;
            }
            candidate.Source = sourceEntry + ":" +
                BuildElementPath(element);
            return true;
        }

        private static void CollectDirectFields(
            XmlElement element,
            Dictionary<string, List<string>> fields)
        {
            foreach (XmlAttribute attribute in element.Attributes)
            {
                AddField(fields, attribute.LocalName, attribute.Value);
            }
            foreach (XmlNode childNode in element.ChildNodes)
            {
                XmlElement child = childNode as XmlElement;
                if (child == null || HasElementChildren(child))
                {
                    continue;
                }
                if (!String.IsNullOrWhiteSpace(child.InnerText)
                    && child.InnerText.Length <= 4096)
                {
                    AddField(fields, child.LocalName, child.InnerText.Trim());
                }
            }
            AddField(fields, "__ElementName", element.LocalName);
        }

        private static int ReadAnchorScore(
            XmlElement element,
            Dictionary<string, List<string>> directFields,
            int id,
            out string evidence)
        {
            evidence = "";
            string idToken = "MiniGame_Fly_" +
                id.ToString(CultureInfo.InvariantCulture);
            int best = 0;

            foreach (KeyValuePair<string, List<string>> pair in directFields)
            {
                foreach (string value in pair.Value)
                {
                    int score = 0;
                    string reason = null;
                    bool exactId = value.IndexOf(
                        idToken,
                        StringComparison.OrdinalIgnoreCase) >= 0;
                    bool nodeClass = value.IndexOf(
                        "DsMiniGameNode_Fly",
                        StringComparison.OrdinalIgnoreCase) >= 0
                        || value.IndexOf(
                            "MiniGameNode_Fly",
                            StringComparison.OrdinalIgnoreCase) >= 0;
                    bool triggerPath = value.IndexOf(
                        "World_MiniGame_Trigger",
                        StringComparison.OrdinalIgnoreCase) >= 0;
                    bool keyAnchor = pair.Key.IndexOf(
                        "Node",
                        StringComparison.OrdinalIgnoreCase) >= 0
                        || pair.Key.IndexOf(
                            "Trigger",
                            StringComparison.OrdinalIgnoreCase) >= 0
                        || pair.Key.IndexOf(
                            "Actor",
                            StringComparison.OrdinalIgnoreCase) >= 0
                        || pair.Key.IndexOf(
                            "NPC",
                            StringComparison.OrdinalIgnoreCase) >= 0
                        || pair.Key.IndexOf(
                            "Class",
                            StringComparison.OrdinalIgnoreCase) >= 0
                        || pair.Key.IndexOf(
                            "Path",
                            StringComparison.OrdinalIgnoreCase) >= 0;

                    if (nodeClass && exactId)
                    {
                        score = 1200;
                        reason = pair.Key + "=DsMiniGameNode_Mole+" + idToken;
                    }
                    else if (nodeClass)
                    {
                        score = 1100;
                        reason = pair.Key + "=DsMiniGameNode_Mole";
                    }
                    else if (triggerPath && exactId)
                    {
                        score = 1050;
                        reason = pair.Key + "=World_MiniGame_Trigger+" + idToken;
                    }
                    else if (exactId && keyAnchor)
                    {
                        score = 900;
                        reason = pair.Key + "=" + idToken;
                    }
                    else if (exactId
                        && (pair.Key.Equals("UIDName", StringComparison.OrdinalIgnoreCase)
                            || pair.Key.Equals("NodeName", StringComparison.OrdinalIgnoreCase)
                            || pair.Key.Equals("Name", StringComparison.OrdinalIgnoreCase)))
                    {
                        score = 850;
                        reason = pair.Key + "=" + idToken;
                    }

                    if (score > best)
                    {
                        best = score;
                        evidence = reason ?? "direct-anchor";
                    }
                }
            }

            if (best == 0
                && element.LocalName.IndexOf(
                    "MiniGameNode_Fly",
                    StringComparison.OrdinalIgnoreCase) >= 0)
            {
                best = 1150;
                evidence = "element=" + element.LocalName;
            }
            return best;
        }

        private static void CollectFields(
            XmlElement element,
            Dictionary<string, List<string>> fields,
            int depth,
            int maxDepth)
        {
            foreach (XmlAttribute attribute in element.Attributes)
            {
                AddField(fields, attribute.LocalName, attribute.Value);
            }
            if (depth >= maxDepth)
            {
                return;
            }

            foreach (XmlNode childNode in element.ChildNodes)
            {
                XmlElement child = childNode as XmlElement;
                if (child == null)
                {
                    continue;
                }

                if (!HasElementChildren(child)
                    && !String.IsNullOrWhiteSpace(child.InnerText)
                    && child.InnerText.Length <= 4096)
                {
                    AddField(fields, child.LocalName, child.InnerText.Trim());
                }
                CollectFields(child, fields, depth + 1, maxDepth);
            }
        }

        private static bool HasElementChildren(XmlElement element)
        {
            foreach (XmlNode child in element.ChildNodes)
            {
                if (child is XmlElement)
                {
                    return true;
                }
            }
            return false;
        }

        private static void AddField(
            Dictionary<string, List<string>> fields,
            string name,
            string value)
        {
            if (String.IsNullOrWhiteSpace(name)
                || String.IsNullOrWhiteSpace(value))
            {
                return;
            }
            List<string> values;
            if (!fields.TryGetValue(name, out values))
            {
                values = new List<string>();
                fields[name] = values;
            }
            if (values.Count < 16)
            {
                values.Add(value.Trim());
            }
        }

        private static bool TryReadCoordinates(
            Dictionary<string, List<string>> fields,
            out double x,
            out double y,
            out double z,
            out bool hasZ,
            out int score)
        {
            x = 0;
            y = 0;
            z = 0;
            hasZ = false;
            score = 0;

            for (int index = 0; index < CoordinateTriplets.Length; index++)
            {
                string[] names = CoordinateTriplets[index];
                string xText = First(fields, names[0]);
                string yText = First(fields, names[1]);
                string zText = First(fields, names[2]);
                if (TryParseNumber(xText, out x)
                    && TryParseNumber(yText, out y))
                {
                    hasZ = TryParseNumber(zText, out z);
                    score = 70 - index * 6 + (hasZ ? 5 : 0);
                    return true;
                }
            }

            foreach (string fieldName in VectorFields)
            {
                List<string> values;
                if (!fields.TryGetValue(fieldName, out values))
                {
                    continue;
                }
                foreach (string value in values)
                {
                    Match match = VectorPattern.Match(value);
                    if (!match.Success
                        || !TryParseNumber(
                            match.Groups["x"].Value,
                            out x)
                        || !TryParseNumber(
                            match.Groups["y"].Value,
                            out y))
                    {
                        continue;
                    }
                    hasZ = match.Groups["z"].Success
                        && TryParseNumber(
                            match.Groups["z"].Value,
                            out z);
                    score = 52 + (hasZ ? 5 : 0);
                    return true;
                }
            }
            return false;
        }

        private static bool TryReadMoleIdentity(
            XmlElement element,
            Dictionary<string, List<string>> fields,
            out int id,
            out string uidName,
            out int score)
        {
            id = 0;
            uidName = null;
            score = 0;
            HashSet<int> detectedIds = new HashSet<int>();
            bool moleEvidence = element.LocalName.IndexOf(
                "Fly",
                StringComparison.OrdinalIgnoreCase) >= 0;

            foreach (KeyValuePair<string, List<string>> pair in fields)
            {
                if (pair.Key.IndexOf(
                        "Fly",
                        StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    moleEvidence = true;
                }
                foreach (string value in pair.Value)
                {
                    if (value.IndexOf(
                            "Fly",
                            StringComparison.OrdinalIgnoreCase) >= 0)
                    {
                        moleEvidence = true;
                    }
                    MatchCollection matches = MoleNamePattern.Matches(value);
                    foreach (Match match in matches)
                    {
                        int matchedId;
                        if (Int32.TryParse(
                            match.Groups[1].Value,
                            NumberStyles.None,
                            CultureInfo.InvariantCulture,
                            out matchedId)
                            && IsMoleId(matchedId))
                        {
                            detectedIds.Add(matchedId);
                            if (String.IsNullOrWhiteSpace(uidName))
                            {
                                uidName = value;
                            }
                        }
                    }
                }
            }
            if (detectedIds.Count > 1)
            {
                return false;
            }
            if (detectedIds.Count == 1)
            {
                id = detectedIds.First();
                score = 110;
                moleEvidence = true;
            }

            int explicitId;
            string miniGameId = First(
                fields,
                "MiniGameID",
                "MiniGameId",
                "MiniGameCID");
            if (TryParseMoleId(miniGameId, out explicitId))
            {
                if (id != 0 && id != explicitId)
                {
                    return false;
                }
                id = explicitId;
                score = Math.Max(score, 100);
                moleEvidence = true;
            }

            if (id == 0 && moleEvidence)
            {
                string[] genericIdFields =
                {
                    "GameID", "ID", "CID", "UID", "NodeID", "TargetID"
                };
                foreach (string fieldName in genericIdFields)
                {
                    if (TryParseMoleId(
                        First(fields, fieldName),
                        out explicitId))
                    {
                        id = explicitId;
                        score = 78;
                        break;
                    }
                }
            }

            if (!IsMoleId(id))
            {
                return false;
            }

            if (String.IsNullOrWhiteSpace(uidName))
            {
                uidName = First(
                    fields,
                    "UIDName",
                    "NodeName",
                    "AssetName",
                    "Name");
            }
            return true;
        }

        private static bool TryReadMapId(
            Dictionary<string, List<string>> fields,
            out int mapId,
            out int score)
        {
            mapId = 0;
            score = 0;
            string[] directNames =
            {
                "MapID",
                "WorldMapID",
                "WorldMapSectionID",
                "MapFieldID"
            };
            foreach (string fieldName in directNames)
            {
                if (TryParsePositiveInt(
                    First(fields, fieldName),
                    out mapId))
                {
                    mapId = NormalizeMapId(mapId);
                    if (mapId > 0)
                    {
                        score = 45;
                        return true;
                    }
                }
            }

            int worldId;
            if (TryParsePositiveInt(First(fields, "WorldID"), out worldId)
                && worldId <= 99)
            {
                mapId = worldId * 100;
                score = 44;
                return true;
            }

            string mapGroup = First(
                fields,
                "MapGroupID",
                "MapGroupId",
                "SectionGroupID");
            long mapGroupValue;
            if (Int64.TryParse(
                    mapGroup,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out mapGroupValue)
                && TryNormalizeMapGroupId(
                    mapGroupValue,
                    out mapId))
            {
                score = 42;
                return true;
            }

            string section = First(fields, "SectionUID", "SectionID");
            long sectionValue;
            if (Int64.TryParse(
                    section,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out sectionValue)
                && sectionValue > 0)
            {
                mapId = NormalizeMapId(sectionValue);
                if (mapId > 0)
                {
                    score = 38;
                    return true;
                }
            }

            string level = First(fields, "LevelCID", "LevelID");
            long levelValue;
            if (Int64.TryParse(
                    level,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out levelValue)
                && levelValue > 0)
            {
                mapId = NormalizeMapId(levelValue);
                if (mapId > 0)
                {
                    score = 30;
                    return true;
                }
            }

            foreach (KeyValuePair<string, List<string>> pair in fields)
            {
                foreach (string value in pair.Value)
                {
                    Match worldMatch = WorldNamePattern.Match(value);
                    int worldNumber;
                    if (worldMatch.Success
                        && Int32.TryParse(
                            worldMatch.Groups["id"].Value,
                            NumberStyles.None,
                            CultureInfo.InvariantCulture,
                            out worldNumber)
                        && worldNumber > 0
                        && worldNumber <= 99)
                    {
                        mapId = worldNumber * 100;
                        score = 27;
                        return true;
                    }

                    Match match = MapNamePattern.Match(value);
                    long matched;
                    if (match.Success
                        && Int64.TryParse(
                            match.Groups["id"].Value,
                            NumberStyles.None,
                            CultureInfo.InvariantCulture,
                            out matched))
                    {
                        mapId = NormalizeMapId(matched);
                        if (mapId > 0)
                        {
                            score = 24;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        private static bool TryNormalizeMapGroupId(
            long value,
            out int mapId)
        {
            mapId = 0;
            if (value <= 0)
            {
                return false;
            }

            // ActorPositionData commonly encodes world map 100 as group 10001,
            // world map 200 as 20001, and so on.
            if (value >= 10000
                && value <= 999999
                && value % 100 > 0)
            {
                long prefix = value / 100;
                if (prefix > 0 && prefix <= 9999)
                {
                    mapId = (int)prefix;
                    return true;
                }
            }

            mapId = NormalizeMapId(value);
            return mapId > 0;
        }

        private static int NormalizeMapId(long value)
        {
            if (value <= 0)
            {
                return 0;
            }
            long normalized = value <= 9999 ? value : value % 1000;
            return normalized > 0 && normalized <= 9999
                ? (int)normalized
                : 0;
        }

        private static string First(
            Dictionary<string, List<string>> fields,
            params string[] names)
        {
            foreach (string name in names)
            {
                List<string> values;
                if (fields.TryGetValue(name, out values)
                    && values.Count > 0)
                {
                    return values[0];
                }
            }
            return null;
        }

        private static bool TryParseMoleId(
            string value,
            out int id)
        {
            if (!Int32.TryParse(
                    value,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out id))
            {
                id = 0;
                return false;
            }
            return IsMoleId(id);
        }

        private static bool IsMoleId(int id)
        {
            return id >= FirstMoleId && id <= LastMoleId;
        }

        private static bool TryParsePositiveInt(
            string value,
            out int parsed)
        {
            return Int32.TryParse(
                    value,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out parsed)
                && parsed > 0;
        }

        private static bool TryParseNumber(
            string value,
            out double parsed)
        {
            return Double.TryParse(
                    value,
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out parsed)
                && IsFinite(parsed);
        }

        private static bool IsFinite(double value)
        {
            return !Double.IsNaN(value) && !Double.IsInfinity(value);
        }

        private static void WriteCandidateReport(
            Dictionary<int, List<MoleCandidate>> candidates,
            string outputPath)
        {
            string directory = Path.GetDirectoryName(outputPath);
            if (String.IsNullOrEmpty(directory))
            {
                return;
            }
            Directory.CreateDirectory(directory);
            string reportPath = Path.Combine(
                directory,
                "mole-anchor-candidates.tsv");
            List<string> lines = new List<string>();
            lines.Add("id	map	x	y	z	has_z	position_role	position_name	anchor_score	score	evidence	source");
            foreach (int id in candidates.Keys.OrderBy(value => value))
            {
                foreach (MoleCandidate candidate in candidates[id]
                    .OrderByDescending(value => value.AnchorScore)
                    .ThenByDescending(value => value.Score))
                {
                    lines.Add(String.Join("\t", new[]
                    {
                        id.ToString(CultureInfo.InvariantCulture),
                        candidate.MapId.ToString(CultureInfo.InvariantCulture),
                        FormatNumber(candidate.X),
                        FormatNumber(candidate.Y),
                        candidate.HasZ ? FormatNumber(candidate.Z) : "",
                        candidate.HasZ ? "true" : "false",
                        (candidate.PositionRole ?? "").Replace("	", " "),
                        (candidate.PositionName ?? "").Replace("	", " "),
                        candidate.AnchorScore.ToString(CultureInfo.InvariantCulture),
                        candidate.Score.ToString(CultureInfo.InvariantCulture),
                        (candidate.AnchorEvidence ?? "").Replace("\t", " "),
                        (candidate.Source ?? "").Replace("\t", " "),
                    }));
                }
            }
            File.WriteAllLines(reportPath, lines, new UTF8Encoding(false));
        }

        private static string DescribeCandidate(MoleCandidate candidate)
        {
            return candidate.Source +
                " [map=" + candidate.MapId.ToString(CultureInfo.InvariantCulture) +
                ", x=" + FormatNumber(candidate.X) +
                ", y=" + FormatNumber(candidate.Y) +
                (candidate.HasZ ? ", z=" + FormatNumber(candidate.Z) : "") +
                ", role=" + (candidate.PositionRole ?? "") +
                ", name=" + (candidate.PositionName ?? "") +
                ", anchor=" + candidate.AnchorScore.ToString(CultureInfo.InvariantCulture) +
                ", score=" + candidate.Score.ToString(CultureInfo.InvariantCulture) +
                ", evidence=" + (candidate.AnchorEvidence ?? "") + "]";
        }

        private static bool SameLocation(
            MoleCandidate left,
            MoleCandidate right)
        {
            return left.MapId == right.MapId
                && Math.Abs(left.X - right.X) <= 0.001
                && Math.Abs(left.Y - right.Y) <= 0.001
                && left.HasZ == right.HasZ
                && (!left.HasZ
                    || Math.Abs(left.Z - right.Z) <= 0.001);
        }

        private static string BuildElementPath(XmlElement element)
        {
            List<string> parts = new List<string>();
            XmlNode current = element;
            while (current is XmlElement && parts.Count < 8)
            {
                parts.Add(((XmlElement)current).LocalName);
                current = current.ParentNode;
            }
            parts.Reverse();
            return String.Join("/", parts.ToArray());
        }

        private static string NormalizeSourceEntry(string path)
        {
            string name = Path.GetFileName(path) ?? String.Empty;
            return Regex.Replace(name, @"^\d+_", String.Empty);
        }

        private static string FormatNumber(double value)
        {
            return value.ToString("R", CultureInfo.InvariantCulture);
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
