#!/bin/bash
set -e

# ── Config ────────────────────────────────────────────────────
MD_FILE="README.md"
SVG_DIR="docs/images"
TEMP_DIR=".tikz_temp"

mkdir -p "$SVG_DIR" "$TEMP_DIR"

echo "🔍 Scanning $MD_FILE for tikz blocks..."

# ── Extract tikz blocks ───────────────────────────────────────
block_index=0
in_block=0
block_content=""

while IFS= read -r line; do
  if [[ "$line" == '```tikz' ]]; then
    in_block=1
    block_content=""

  elif [[ "$line" == '```' && $in_block -eq 1 ]]; then
    in_block=0
    block_index=$((block_index + 1))
    tex_file="$TEMP_DIR/circuit_${block_index}.tex"
    svg_file="$SVG_DIR/circuit_${block_index}.svg"

    echo "📐 Rendering block $block_index..."

    cat > "$tex_file" <<EOF
\documentclass[tikz, border=10pt]{standalone}
\usepackage{circuitikz}
\begin{document}
$block_content
\end{document}
EOF

    # Compile to DVI then SVG
    latex -interaction=nonstopmode -output-directory="$TEMP_DIR" "$tex_file" > /dev/null 2>&1
    dvisvgm --font-format=woff \
            --output="$svg_file" \
            "$TEMP_DIR/circuit_${block_index}.dvi"

    # ── Inject dark mode CSS into the SVG ────────────────────
    python3 - "$svg_file" <<'PYEOF'
import sys
import re

svg_path = sys.argv[1]

with open(svg_path, "r") as f:
    svg = f.read()

# CSS that inverts colors in dark mode
dark_mode_css = """<style>
  /* Light mode: default black lines on white */
  svg {
    background-color: transparent;
  }
  /* Dark mode: invert all colors */
  @media (prefers-color-scheme: dark) {
    svg {
      filter: invert(1) hue-rotate(180deg);
    }
  }
</style>"""

# Inject the <style> block right after the opening <svg ...> tag
svg = re.sub(r'(<svg[^>]*>)', r'\1' + dark_mode_css, svg, count=1)

with open(svg_path, "w") as f:
    f.write(svg)

print(f"🌙 Dark mode CSS injected into {svg_path}")
PYEOF

    echo "✅ Saved: $svg_file"

  elif [[ $in_block -eq 1 ]]; then
    if [[ "$line" != *"\usepackage"* && \
          "$line" != *"\begin{document}"* && \
          "$line" != *"\end{document}"* ]]; then
      block_content+="$line"$'\n'
    fi
  fi
done < "$MD_FILE"

# ── Update README with image references ───────────────────────
echo "📝 Updating $MD_FILE with image links..."

python3 - <<'PYEOF'
import re

with open("README.md", "r") as f:
    content = f.read()

index = [0]

def replace_block(match):
    index[0] += 1
    svg_path = f"docs/images/circuit_{index[0]}.svg"
    return f"![Circuit Diagram {index[0]}]({svg_path})"

updated = re.sub(r'```tikz.*?```', replace_block, content, flags=re.DOTALL)

with open("README.md", "w") as f:
    f.write(updated)

print(f"✅ Replaced {index[0]} tikz block(s) in README.md")
PYEOF

# ── Cleanup ───────────────────────────────────────────────────
rm -rf "$TEMP_DIR"
echo "🎉 Done! All circuits rendered."