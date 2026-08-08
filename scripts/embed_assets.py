"""
PlatformIO pre-build extra_script.
Reads assets/index.html and generates include/html_assets.h
as a C++ raw-string constant placed in Flash (.rodata).
Run automatically before every build — do not edit the .h directly.
"""
import os
Import("env")


def embed_html(source, target, env):
    project = env["PROJECT_DIR"]
    src = os.path.join(project, "assets", "index.html")
    dst = os.path.join(project, "include", "html_assets.h")
    with open(src, encoding="utf-8") as f:
        html = f.read()
    # Ensure no raw ")HTML" sequence inside the HTML breaks the delimiter
    if ")HTML\"" in html:
        raise RuntimeError(
            "assets/index.html contains the forbidden sequence )HTML\" — rename the raw-string delimiter")
    with open(dst, "w", encoding="utf-8") as f:
        f.write(
            "// Auto-generated from assets/index.html — edit the source, not this file.\n")
        f.write("#pragma once\n\n")
        f.write("static const char INDEX_HTML[] = R\"HTML(\n")
        f.write(html)
        f.write("\n)HTML\";\n")
    print("[embed_assets] %s -> %s  (%d bytes)" % (src, dst, len(html)))


env.AddPreAction("$BUILD_DIR/src/wifi_network.cpp.o", embed_html)
