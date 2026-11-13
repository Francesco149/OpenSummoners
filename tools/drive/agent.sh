#!/bin/sh
# An API-billed driver agent (tools/drive/agent.py) in its own venv. Usage: tools/drive/agent.sh TASK.md --run NAME [--budget N] [--model M]
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
VENV=${RD_VENV:-/mnt/ephemeral/opensummoners/drive/venv}
if [ ! -x "$VENV/bin/python" ]; then
    uv venv --python 3.13 "$VENV" >&2
    uv pip install --python "$VENV/bin/python" "anthropic>=2.6" pillow >&2
fi
case "$1" in check) shift; exec "$VENV/bin/python" -u "$HERE/api_check.py" "$@";; esac
exec "$VENV/bin/python" -u "$HERE/agent.py" "$@"
