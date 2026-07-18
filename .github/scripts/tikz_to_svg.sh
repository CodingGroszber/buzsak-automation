#!/bin/bash
set -e

# ── Config ────────────────────────────────────────────────────
MD_FILE="README.md"
SVG_DIR="docs/images"
TEMP_DIR=".tikz_temp"

mkdir -p "$SVG_DIR" "$TEMP_DIR"

echo "🔍 Scanning $MD_FILE for tikz blocks..."

# ── Extract tikz blocks using awk ─────────────────────────────
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

    # Write standalone LaTeX file
    cat > "$tex_file" <<EOF
\documentclass[tikz, border=10pt]{standalone}
\usepackage{circuitikz}
\begin{document}
$block_content
\end{document}
EOF

    # Compile to DVI then convert to SVG
    latex -interaction=nonstopmode -output-directory="$TEMP_DIR" "$tex_file" > /dev/null 2>&1
    dvisvgm --font-format=woff \
            --output="$svg_file" \
            "$TEMP_DIR/circuit_${block_index}.dvi"

    echo "✅ Saved: $svg_file"

  elif [[ $in_block -eq 1 ]]; then
    # Skip the \usepackage and \begin/\end{document} lines
    # since standalone handles that
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

# Replace ```tikz ... ``` blocks with image references
updated = re.sub(r'```tikz.*?```', replace_block, content, flags=re.DOTALL)

with open("README.md", "w") as f:
    f.write(updated)

print(f"✅ Replaced {index[0]} tikz block(s) in README.md")
PYEOF

# ── Cleanup ───────────────────────────────────────────────────
rm -rf "$TEMP_DIR"
echo "🎉 Done! All circuits rendered."