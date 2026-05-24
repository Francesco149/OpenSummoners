# Apply class namespace + __thiscall + signature to a fixed list of
# functions, so the decompile shows `this->field` reads matching our
# C ports.
#
# Run this in the GUI via Script Manager (Window -> Script Manager,
# refresh, find TagThiscallFunctions, double-click), or headlessly
# via:
#
#   ghidra-analyzeHeadless ghidra/projects opensummoners \
#       -process sotes.unpacked.exe \
#       -noanalysis \
#       -scriptPath tools/ghidra-scripts \
#       -postScript TagThiscallFunctions.py
#
# PREREQUISITES (one-time per project):
#   1. Parse C Source on src/asset_register.h (GUI: File -> Parse C
#      Source) so that `ar_sprite_slot`, `ar_gdi_slot`, and
#      `ar_sound_slot` exist in the Data Type Manager.  Without these
#      structs, the auto-this on the class namespace falls back to
#      `void *` and the decompile improvement is much smaller.
#
#   2. Make sure no other instance has the project open with a write
#      lock (the GUI editor and headless can't both hold the lock at
#      once).
#
# IDEMPOTENT: re-running over an already-tagged project is safe; each
# step is a no-op when the target state already matches.
#
# WHY: see docs/findings/cpp-recovery-workflow.md.

#@category OpenSummoners
#@menupath Tools.OpenSummoners.Tag Thiscall Functions

from ghidra.app.util.parser import FunctionSignatureParser
from ghidra.app.services import DataTypeManagerService
from ghidra.app.cmd.function import ApplyFunctionSignatureCmd
from ghidra.program.model.symbol import SourceType


# ─── the table ─────────────────────────────────────────────────────────────
#
# (address, class_name, prototype) — one entry per ported thiscall.
#
# The class_name MUST match a struct of the same name in the Data Type
# Manager (Ghidra correlates the class namespace to the struct by name
# when generating the auto-this parameter).
#
# The prototype is the explicit-args-only form: no `this`, no
# `__thiscall` keyword.  Both are added automatically by Ghidra once
# the function lives in a class namespace with `__thiscall` convention.
#
# Source of truth for the prototypes is src/asset_register.h; this
# table lives in the script (not externalised) because Jython has no
# YAML and the readability win of literal Python beats a JSON sidecar
# for ~10 entries.  Bump this table whenever a new thiscall is ported.

THISCALL_TAGS = [
    # Asset-Register module
    (0x005748c0, "ar_sprite_slot",
        "undefined4 FUN_005748c0(void * zdd, void * settings, ushort resource_id, "
        "uint width, uint height, uint colorkey, uint scale_flag, uint type, ushort group)"),
    (0x00417b50, "ar_sprite_slot",
        "void FUN_00417b50(void)"),
    (0x00562a10, "ar_gdi_slot",
        "void FUN_00562a10(void)"),
    (0x00579ec0, "ar_gdi_slot",
        "void FUN_00579ec0(ushort capacity)"),
    (0x0057a030, "ar_gdi_slot",
        "void FUN_0057a030(int width, int height, uint family, ushort group)"),
    (0x0057a1a0, "ar_gdi_slot",
        "void FUN_0057a1a0(int width, uint color, ushort group, ushort capacity)"),
    (0x0057a260, "ar_gdi_slot",
        "void FUN_0057a260(uint color, ushort group, ushort capacity)"),
    (0x00563ef0, "ar_sound_slot",
        "undefined4 FUN_00563ef0(void * zds, void * settings, ushort resource_id, "
        "ushort count, ushort group, int load_flag)"),
]


# ─── implementation ────────────────────────────────────────────────────────

prog = currentProgram
st   = prog.getSymbolTable()
dtm  = prog.getDataTypeManager()

# DataTypeManagerService is None under headless — the parser works without
# it (just won't auto-resolve types from the GUI's open archives, which is
# fine because our types come from the program's own DTM after Parse C
# Source ingested asset_register.h).
dtms = state.tool.getService(DataTypeManagerService) if state.tool else None
parser = FunctionSignatureParser(dtm, dtms)


def get_or_create_class(name):
    """Return the class namespace `name` under global, creating if needed."""
    parent = prog.getGlobalNamespace()
    ns = st.getNamespace(name, parent)
    if ns is not None:
        return ns
    return st.createClass(parent, name, SourceType.USER_DEFINED)


def tag_function(addr_int, class_name, prototype):
    addr = toAddr(addr_int)
    func = getFunctionAt(addr)
    if func is None:
        printerr("  no function at 0x%08x — skip" % addr_int)
        return False

    # Step 1: move into the class namespace.  Auto-this resolves to
    # `<class_name> *` IFF a struct of the same name exists in the DTM.
    cls = get_or_create_class(class_name)
    if func.getParentNamespace() != cls:
        func.setParentNamespace(cls)

    # Step 2: __thiscall.  Setting before the signature apply avoids
    # the "auto params can't be edited" trap.
    if func.getCallingConventionName() != "__thiscall":
        func.setCallingConvention("__thiscall")

    # Step 3: parse + apply the typed prototype.  ApplyFunctionSignatureCmd
    # is the same path the GUI's Edit Function Signature dialog uses, so
    # this is exactly equivalent to clicking through.
    try:
        sig = parser.parse(func.getSignature(), prototype)
    except Exception as e:
        printerr("  parse failed for 0x%08x: %s" % (addr_int, e))
        return False

    cmd = ApplyFunctionSignatureCmd(addr, sig, SourceType.USER_DEFINED)
    if not cmd.applyTo(prog, monitor):
        printerr("  apply failed for 0x%08x: %s" % (addr_int, cmd.getStatusMsg()))
        return False

    print("  tagged %s::%s @ 0x%08x" % (class_name, func.getName(), addr_int))
    return True


print("Tagging %d thiscall functions:" % len(THISCALL_TAGS))
ok = 0
for addr_int, class_name, prototype in THISCALL_TAGS:
    if tag_function(addr_int, class_name, prototype):
        ok += 1
print("\nResult: %d/%d tagged" % (ok, len(THISCALL_TAGS)))
