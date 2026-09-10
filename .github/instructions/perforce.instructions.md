---
applyTo: '**'
---

# Perforce (P4) Checkout

This repository is tracked in a Perforce depot, so files are typically **read-only** by default.

Before editing any tracked file, run `p4 edit <file>` (or `p4 checkout <file>` if using a P4 tool that supports it) on the file's absolute path first, so the file is made writable and opened for edit in the default changelist. Do this proactively whenever an edit attempt fails due to a read-only file, or preemptively before editing if you know the file is under source control.
