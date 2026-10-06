#! /usr/bin/bash
if command -v tmux &> /dev/null; then

    tmux new-session "./build/server/PubSub; read" \; split-window "./build/examples/pubMult; read" \; split-window "./build/examples/subMult; read" \; set-window-option main-pane-width 65% \; select-layout main-vertical

else
echo "Tmux is not Installed. running in same shell"
./build/server/PubSub &
./build/examples/pubMult &
./build/examples/subMult &
fi