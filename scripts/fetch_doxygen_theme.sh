#!/bin/sh
# scripts/fetch_doxygen_theme.sh
# Fetch the doxygen-awesome-css theme into docs/doxygen-awesome-css
set -e
mkdir -p docs
if [ -d docs/doxygen-awesome-css ]; then
  echo "doxygen-awesome-css already exists"
  exit 0
fi
# Clone a shallow copy of the theme
git clone --depth 1 https://github.com/jothepro/doxygen-awesome-css.git docs/doxygen-awesome-css
echo "Fetched doxygen-awesome-css into docs/doxygen-awesome-css"
