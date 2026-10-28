#!/bin/sh
set -eu
task_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$task_root"
mkdir -p build/native build/results
task_cc=${CC:-clang}
task_python=${PYTHON:-python3}
"$task_cc" -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Ifirmware tests/test_station_logic.c firmware/station_logic.c -o build/native/test-state
build/native/test-state
"$task_cc" -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Ifirmware -Imacos tests/test_music_contract.c firmware/station_logic.c -o build/native/test-contract
build/native/test-contract
"$task_cc" -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imacos macos/test_link_protocol.c -o build/native/test-link
build/native/test-link
"$task_python" tests/test_display.py
"$task_python" tests/test_right_overlay.py
"$task_python" tests/test_typing_adapter.py
"$task_cc" -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Ifirmware tests/test_environment.c firmware/station_display.c -o build/native/test-environment
build/native/test-environment
