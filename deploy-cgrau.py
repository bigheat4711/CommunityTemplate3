import os, zipfile, shutil
from pathlib import Path

# Get the version number from the build environment.
firmware_version = os.environ.get('VERSION', "")
if firmware_version == "":
    firmware_version = "0.0.1"
firmware_version = firmware_version.lstrip("v")
firmware_version = firmware_version.strip(".")

# Get the ZIP filename from the build environment.
#community_project = env.GetProjectOption('custom_community_project', "")

# Get the custom folder from the build environment.
#custom_source_folder = env.GetProjectOption('custom_source_folder', "")

# Get the foldername inside the zip file from the build environment.
#zip_filename = env.GetProjectOption('custom_zip_filename', "")

#platform = env.BoardConfig().get("platform", {})

shutil.copytree("./board1/Community", "C:/Users/h5/AppData/Local/MobiFlight/MobiFlight Connector/Community/board1", dirs_exist_ok=True);