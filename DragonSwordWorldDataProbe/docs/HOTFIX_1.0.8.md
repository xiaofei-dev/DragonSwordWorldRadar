# 1.0.8 hotfix

The uploaded 1.0.7 diagnostic confirms the visible-window storm was caused by
the Mole result-writing path launching a shell to ensure directories for every
result. The Mole scan starts around one minute after entering the game, matching
the reported timing.

The completion query itself is valid: 12001 through 12018 returned booleans.

1.0.8 removes all shell/external-process creation from the Mole runtime module.
It writes directly to the pre-existing runtime/reports directory and fails safely
if that directory cannot be opened.
