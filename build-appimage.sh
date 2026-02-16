#!/bin/bash
set -e

PROJECT_NAME="AniStudio"
PROJECT_VERSION="0.2.0"
ROOT_DIR="$(pwd)"
BUILD_DIR="${ROOT_DIR}/build-appimage"
APP_DIR="${BUILD_DIR}/AppDir"
ADDON_PATH="${ROOT_DIR}/addons/Diffusionaddon"  # CORRECT PATH

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# INSTALL CONAN FOR MAIN PROJECT
conan install "${ROOT_DIR}" \
    --build=missing \
    --output-folder="${BUILD_DIR}"

# COPY CONAN FILES
mkdir -p "${BUILD_DIR}/conan"
cp -r "${BUILD_DIR}"/*.cmake "${BUILD_DIR}/conan/" 2>/dev/null || true
cp -r "${BUILD_DIR}"/build/conan/* "${BUILD_DIR}/conan/" 2>/dev/null || true

# BUILD MAIN ANISTUDIO
echo "Building main AniStudio..."
cmake "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

make -j$(nproc)
make install DESTDIR=AppDir

# FIX LIBRARY NAMES
if [ -f "${APP_DIR}/usr/lib/libAniEngineCore.so" ]; then
    ln -sf "libAniEngineCore.so" "${APP_DIR}/usr/lib/AniEngineCore.so"
fi
if [ -f "${APP_DIR}/usr/lib/libAniStudioCore.so" ]; then
    ln -sf "libAniStudioCore.so" "${APP_DIR}/usr/lib/AniStudioCore.so"
fi

# BUILD DIFFUSION ADDON
if [ -d "${ADDON_PATH}" ]; then
    echo "Building DiffusionAddon from ${ADDON_PATH}..."
    mkdir -p "${BUILD_DIR}/addons/Diffusionaddon"
    cd "${BUILD_DIR}/addons/Diffusionaddon"
    
    # Build with all backends
    cmake "${ADDON_PATH}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSD_BUILD_ALL=ON
    
    make -j$(nproc)
    
    # Copy addons to AppDir
    mkdir -p "${APP_DIR}/usr/lib/${PROJECT_NAME}/addons"
    
    # Find and copy all .so files
    find "${BUILD_DIR}/addons/Diffusionaddon" -name "*.so" -exec cp {} "${APP_DIR}/usr/lib/${PROJECT_NAME}/addons/" \; 2>/dev/null || true
    find "${ROOT_DIR}/build/addons" -name "*.so" -exec cp {} "${APP_DIR}/usr/lib/${PROJECT_NAME}/addons/" \; 2>/dev/null || true
    
    echo "Addons installed:"
    ls -la "${APP_DIR}/usr/lib/${PROJECT_NAME}/addons/" || true
    
    cd "${BUILD_DIR}"
else
    echo "WARNING: Addon not found at ${ADDON_PATH}"
fi

# Download linuxdeploy
cd "${BUILD_DIR}"
wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# Create AppRun
cat > "${APP_DIR}/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="${HERE}/usr/bin/:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib/:${HERE}/usr/lib/AniStudio/addons/:${LD_LIBRARY_PATH}"
export ANISTUDIO_ADDONS_DIR="${HERE}/usr/lib/AniStudio/addons"
exec "${HERE}/usr/bin/AniStudio" "$@"
EOF
chmod +x "${APP_DIR}/AppRun"

# Create desktop file
mkdir -p "${APP_DIR}/usr/share/applications"
cat > "${APP_DIR}/usr/share/applications/anistudio.desktop" << EOF
[Desktop Entry]
Name=AniStudio
Exec=AniStudio
Icon=anistudio
Type=Application
Categories=Graphics;
EOF

# Create icon
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/256x256/apps"
touch "${APP_DIR}/usr/share/icons/hicolor/256x256/apps/anistudio.png"

# Make AppImage
export LD_LIBRARY_PATH="${APP_DIR}/usr/lib:${APP_DIR}/usr/lib/AniStudio/addons:${LD_LIBRARY_PATH}"
./linuxdeploy-x86_64.AppImage --appdir=AppDir --output appimage

# Move final AppImage
mv AniStudio-*.AppImage ../ 2>/dev/null || true

echo "========================================="
echo "AppImage created: $(ls -1 ../AniStudio-*.AppImage)"
echo "Addons: $(ls -1 ${APP_DIR}/usr/lib/AniStudio/addons/ 2>/dev/null | wc -l) files"
echo "========================================="