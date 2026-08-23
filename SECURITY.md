Security Policy
Supported Versions
Version
Supported
main branch (current)
✅ Active
Any tagged release
✅ Active
Archived branches
❌ No fixes backported
Reporting a Vulnerability
Do not open a public GitHub Issue for security vulnerabilities.
Use GitHub's private vulnerability reporting:
Security → Report a vulnerability in this repository's tab bar.
Alternatively, contact the maintainer directly via the contact
information in the repository profile.
What to include
A useful report covers:
Which component is affected (core.hpp, a workflow file,
a shell script, the Docker configuration, etc.)
A description of the vulnerability and its impact
Steps to reproduce or a proof-of-concept (a minimal test
case is more actionable than a description alone)
Your assessment of severity (see scope table below)
Whether you believe the issue is already being exploited
What happens next
Timeframe
Action
Within 48 hours
Acknowledgement of the report
Within 7 days
Initial triage and severity classification
Within 30 days
Patch or documented mitigation for confirmed findings
At patch release
Credit to reporter (unless anonymity is requested)
These are commitments, not guarantees — this is a solo-maintained
project. If a finding is complex, timelines will be communicated
honestly rather than silently missed.
Scope
In scope
Area
Examples
Process isolation bypass
Escaping the forked child boundary in core.hpp
Path traversal
Defeating Moltbook's weakly_canonical boundary
Quarantine bypass
Causing a quarantined mutation to execute anyway
Cross-process lock bypass
Defeating CrossProcessGuard flock serialization
Hive Mind injection
Importing executable content via peer CSV sync
Archive poisoning
Bypassing safe_extract.py allow-list
Token leakage
GitHub App token accessible beyond intended scope
Branch protection bypass
Bot push succeeding on protected pipeline files
Kill switch circumvention
Autonomous code disabling KILL_SWITCH.lock detection
Out of scope
Area
Reason
GitHub infrastructure
Not in our control
Runner host kernel exploits
Requires privileged runner access
Attacks requiring repo admin credentials
Out of threat model scope
Social engineering
Not a technical vulnerability
Denial-of-service against GitHub API
Third-party service
Safe harbour
Security research on this project conducted in good faith is
welcome. Testing against your own fork is strongly preferred over
testing against the main repository. Any testing that generates
excessive GitHub API calls, triggers billing events, or disrupts
other contributors' workflows is outside safe harbour.
Known Open Findings
These are documented publicly because transparency builds trust.
They are tracked as engineering action items, not hidden.
ID
Severity
Description
Status
F-05
Medium
Quarantine does not persist across CI runs (/tmp is ephemeral)
Open — partially mitigated by Hive Mind sync
For the full audit history, see SECURITY_AUDIT.md.
Security Architecture Summary
SWAYAM's threat model assumes the primary adversary is its own
generated output — a mutation that is malicious, memory-unsafe,
or attempts to escape its execution boundary.
The defenses in depth:
SafeShell — static pattern rejection before compilation
HeuristicAnalyzer — 3-pass native static analysis,
pre-quarantine before disk write
Moltbook — two-layer path traversal prevention
Docker sandbox — --network none, --read-only, non-root
MutationLease — RAII in-process atomic + cross-process flock
Crash-safe supervisor — fork boundary isolates crashes
QuarantineRegistry — FNV-1a hash ledger, atomic writes
Hive Mind — distributed quarantine with DoS defense
CODEOWNERS + branch protection — pipeline self-modification prevention
Full architectural detail: README.md
Full audit report: SECURITY_AUDIT.md
