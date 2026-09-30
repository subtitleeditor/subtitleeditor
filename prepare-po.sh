#!/bin/sh
# Regenerate po/POTFILES.in with every file that can contain translatable strings
{
   find share -name "*.desktop.in"
   find share/ui -name "*.ui"
   find src -name "*.h" -o -name "*.cc"
   find plugins -name "*.h" -o -name "*.cc" -o -name "*.ui" -o -name "*.se-plugin.desktop.in" -o -name "*.se-pattern"
} | sed -e "s/^\.\///" | sort > po/POTFILES.in
