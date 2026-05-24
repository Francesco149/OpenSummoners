/*
 * dev_hooks — developer-time process instrumentation.
 *
 * These hooks are intentionally NOT part of the drop-in replication —
 * they make OpenSummoners friendlier to run under the agent / CI /
 * WSL harness, where a modal dialog stops the world without surfacing
 * anything to stderr.  In a real drop-in shipping scenario you'd
 * probably want to leave them off (pass --show-msgbox).
 *
 * Hooks currently installed:
 *   - user32!MessageBoxA  → log to stderr + return IDOK
 *   - user32!MessageBoxW  → log to stderr (UTF-16 → UTF-8) + return IDOK
 *
 * The hook returns IDOK unconditionally so any "did the user click Yes?"
 * gate always takes the positive branch.  If a popup needs a different
 * default we'll add a per-call table later.
 *
 * Why: a sibling-project session (OpenMare SGL bring-up) burned ~30 min
 * on a "setdisplaymode failed" MessageBox the renderer DLL fires
 * internally — completely opaque from agent-mode shell output until a
 * human noticed the dialog on screen.  The hook makes that failure mode
 * stand out instead of hiding the entire process behind a modal.
 */
#ifndef OPENSUMMONERS_DEV_HOOKS_H
#define OPENSUMMONERS_DEV_HOOKS_H

/* Patch user32!MessageBoxA / MessageBoxW so calls land in stderr and
 * return IDOK.  Returns 1 on full success, 0 on any patch failure (with
 * details to stderr).  Idempotent. */
int dev_hooks_install_messagebox(void);

/* Restore the original prologues.  Safe to call even if install failed
 * or wasn't called. */
void dev_hooks_uninstall_messagebox(void);

#endif /* OPENSUMMONERS_DEV_HOOKS_H */
