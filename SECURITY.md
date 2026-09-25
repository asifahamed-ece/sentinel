# Security Policy

SENTINEL is a safety-critical IoT system. We take security reports
seriously and appreciate your help in keeping workers and deployments
safe.

## Supported Versions

Only the latest release on the `main` branch receives security fixes.
Pre-release, archived, or experimental firmware is **not** supported.

| Version | Supported          |
|:--------|:-------------------|
| 6.x (latest) | ✅ Security fixes |
| < 6.0 | ❌ Unsupported |

## Reporting a Vulnerability

**Please do NOT open a public issue for security vulnerabilities.**

Instead, report privately using the preferred path below:

1. **GitHub private security advisory (recommended)**

   Go to the repository's **Security** tab → *Report a vulnerability*,
   and fill in the advisory form. This keeps the report private until a
   fix is released.

2. **Direct email to a maintainer**

   If the advisory form is unavailable, contact the maintainers directly
   via the email addresses on their GitHub profiles.

### What to include in a report

To help us triage quickly, include:

- Affected component: **firmware (node/hub)** or **dashboard (web)**
- Hardware/board and firmware version (e.g., `v6.4`, `Admin_v6`)
- Steps to reproduce, including any serial/log output
- Impact: what an attacker could achieve (e.g., spoofed packet, alert
  suppression, crash/RF denial)
- Any proposed fix or mitigation, if you have one

### What happens next

- We will acknowledge your report within **72 hours**.
- We will provide an estimated timeline for a fix and keep you informed
  as a fix is developed.
- Once a patch is ready and released, we will publish an advisory
  (academic/educational context allowing) and credit the reporter unless
  they prefer to remain anonymous.

## Security notes for this project

- ESP-NOW frames in this reference implementation are **not
  authenticated or encrypted**. This is an academic prototype; for
  production/SME deployment, wrap the link with ESP32 WPA2 or an
  application-layer signing scheme before relying on alert integrity.
- The Hub broadcasts over WebSockets on a WiFi AP without authentication.
  Restrict physical/network access to the dashboard deployment.
- Default credentials (e.g., `AP_PASS`, `HUB_MAC`) are development
  defaults — change them before any real deployment.