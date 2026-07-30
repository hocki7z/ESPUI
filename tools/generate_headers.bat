@echo off
REM Regenerate ESPUI C header files (including gzip) from source JS/CSS files.
REM Run this after modifying any file in data/js/ or data/css/

cd /d "%~dp0.."

echo Installing required Python packages if missing...
python -m pip install jsmin htmlmin csscompressor --quiet

echo Generating header files...
python tools/prepare_static_ui_sources.py --auto

echo Done. Header files updated in src/
pause
