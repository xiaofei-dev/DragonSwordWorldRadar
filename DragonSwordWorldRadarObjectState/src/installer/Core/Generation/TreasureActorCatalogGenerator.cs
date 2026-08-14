using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    public static class TreasureActorCatalogGenerator
    {
        private static readonly HashSet<string> SupportedClasses =
            new HashSet<string>(StringComparer.Ordinal)
            {
                "TreasureBox01_C",
                "TreasureBox02_C",
                "TreasureBox02_Mount_C",
                "TreasureBox03_C",
                "TreasureBox03_Mount_C",
                "TreasureBox03_OnlyFront_C",
                "TreasureBox04Key_C",
                "TreasureBox04_C",
                "TreasureBox05_C",
                "TreasureBox05_Mount_C",
                "TreasureBox06_C"
            };

        public static int Generate(
            string sectionXmlPath,
            string propXmlPath,
            string outputPath)
        {
            XmlDocument propDocument = Load(propXmlPath);
            XmlNodeList propNodes = propDocument.SelectNodes(
                "//*[local-name()='PropTreasureBoxData']");
            if (propNodes == null || propNodes.Count < 1000)
            {
                throw new InvalidDataException(
                    "The extracted treasure blueprint data is incomplete.");
            }
            Dictionary<string, string> classesById =
                new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (XmlNode node in propNodes)
            {
                XmlElement element = node as XmlElement;
                if (element == null)
                {
                    continue;
                }
                string id = Attribute(element, "ID");
                string className = GeneratedClassName(
                    Attribute(element, "BluePrintPath"));
                if (String.IsNullOrWhiteSpace(id)
                    || !SupportedClasses.Contains(className))
                {
                    continue;
                }
                string existing;
                if (classesById.TryGetValue(id, out existing)
                    && !String.Equals(
                        existing,
                        className,
                        StringComparison.Ordinal))
                {
                    throw new InvalidDataException(
                        "Treasure blueprint ID maps to multiple generated classes: " +
                        id);
                }
                classesById[id] = className;
            }

            XmlDocument sectionDocument = Load(sectionXmlPath);
            XmlNodeList sectionNodes = sectionDocument.SelectNodes(
                "//*[local-name()='SectionActorData']");
            if (sectionNodes == null || sectionNodes.Count < 1000)
            {
                throw new InvalidDataException(
                    "The extracted treasure actor data is incomplete.");
            }
            List<string> lines = new List<string>
            {
                "SaveId\tClassName\tX\tY\tZ"
            };
            int unresolved = 0;
            foreach (XmlNode node in sectionNodes)
            {
                XmlElement element = node as XmlElement;
                if (element == null)
                {
                    continue;
                }
                string saveId = Attribute(element, "CID");
                string className;
                if (String.IsNullOrWhiteSpace(saveId)
                    || !classesById.TryGetValue(saveId, out className))
                {
                    unresolved++;
                    continue;
                }
                string x = RequiredNumber(element, "PosX");
                string y = RequiredNumber(element, "PosY");
                string z = RequiredNumber(element, "PosZ");
                long parsedSaveId;
                if (!Int64.TryParse(
                    saveId,
                    NumberStyles.None,
                    CultureInfo.InvariantCulture,
                    out parsedSaveId)
                    || parsedSaveId <= 0)
                {
                    throw new InvalidDataException(
                        "Treasure CID contains an invalid value.");
                }
                lines.Add(String.Join(
                    "\t",
                    saveId,
                    className,
                    x,
                    y,
                    z));
            }
            int resolved = lines.Count - 1;
            if (resolved < 1600
                || resolved > 2500
                || unresolved > 2)
            {
                throw new InvalidDataException(String.Format(
                    CultureInfo.InvariantCulture,
                    "Treasure actor join is incomplete: resolved={0}; unresolved={1}.",
                    resolved,
                    unresolved));
            }
            File.WriteAllLines(
                outputPath,
                lines,
                new UTF8Encoding(false));
            return resolved;
        }

        private static XmlDocument Load(string path)
        {
            XmlDocument document = new XmlDocument
            {
                XmlResolver = null
            };
            document.Load(path);
            return document;
        }

        private static string Attribute(
            XmlElement element,
            string localName)
        {
            foreach (XmlAttribute attribute in element.Attributes)
            {
                if (String.Equals(
                    attribute.LocalName,
                    localName,
                    StringComparison.OrdinalIgnoreCase))
                {
                    return attribute.Value == null
                        ? String.Empty
                        : attribute.Value.Trim();
                }
            }
            return String.Empty;
        }

        private static string GeneratedClassName(string path)
        {
            if (String.IsNullOrWhiteSpace(path))
            {
                return String.Empty;
            }
            string normalized = path.Trim()
                .Replace('\\', '/');
            int quote = normalized.IndexOf('\'');
            if (quote >= 0)
            {
                normalized = normalized.Substring(quote + 1)
                    .TrimEnd('\'');
            }
            int slash = normalized.LastIndexOf('/');
            string tail = slash >= 0
                ? normalized.Substring(slash + 1)
                : normalized;
            int dot = tail.LastIndexOf('.');
            string objectName = dot >= 0
                ? tail.Substring(dot + 1)
                : tail;
            return objectName.EndsWith(
                "_C",
                StringComparison.Ordinal)
                    ? objectName
                    : objectName + "_C";
        }

        private static string RequiredNumber(
            XmlElement element,
            string localName)
        {
            string value = Attribute(element, localName);
            double parsed;
            if (!Double.TryParse(
                    value,
                    NumberStyles.Float,
                    CultureInfo.InvariantCulture,
                    out parsed)
                || Double.IsNaN(parsed)
                || Double.IsInfinity(parsed))
            {
                throw new InvalidDataException(
                    localName + " contains an invalid value.");
            }
            return value;
        }
    }
}
