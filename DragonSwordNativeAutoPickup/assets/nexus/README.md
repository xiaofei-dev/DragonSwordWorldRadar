# AutoPickup 1.3.1 - Nexus Publishing Index

Use this directory's current text as one set. No upload or post has been made
by preparing these files.

- `NEXUS_DESCRIPTION.txt`: evergreen whole-Mod description (BBCode): features,
  installation, controls, configuration, limitations, and support. Keep Mod
  release numbers, release announcements, and change history out of this file;
  required dependency versions remain where necessary for correct installation.
- `NEXUS_FAQ.txt`: evergreen, compact pinned Quick Support (BBCode). No fixed
  Mod release number or changelog; direct users to the latest package without
  implying that every historical version supports the current features.
- `NEXUS_CHANGELOG.txt`: plain-text changelog; select the 1.3.1 section.
- `NEXUS_FILES.txt`: exact archive-to-display-name mapping and file descriptions.

Each file-description field has a 255-character limit. Copy only the text
after `Description:`, not the label or archive metadata. Check the complete
rendered final sentence after saving. Full description and Quick Support use
their own editors; paste the complete BBCode, not a file-description field.

Before publishing, compare the local ZIP hashes with the product's current
release manifest and read [release status](../../docs/RELEASE_STATUS.md).
Use 1.3.1 in the Nexus version field. Existing screenshots and artwork may
carry older version numbers: retain their historical identity, and replace
them only with genuine new-version captures. Do not rename old captures as
proof of current UI or runtime behavior.

Quick Support intentionally omits internal implementation history. It retains
setup, controls, settings, troubleshooting, and safe log-sharing guidance.
Unverified compatibility or all-device runtime-fix claims must not be added.
Third-party payload and catalog redistribution remains subject to its recorded
rights review; writing public copy does not clear it.
