on run argv
  set VOLUME_NAME to item 1 of argv
  set PROJECT_NAME to item 2 of argv

	tell application "Finder"
		tell disk VOLUME_NAME
			open

			set current view of container window to icon view
			set toolbar visible of container window to false
			set statusbar visible of container window to false

			set the bounds of container window to {100, 100, 820, 640}

			set viewOptions to the icon view options of container window
			set arrangement of viewOptions to not arranged
			set icon size of viewOptions to 128

			set background picture of viewOptions to file ".background:background.png"

			set APP_FILE_NAME to PROJECT_NAME & ".app"
			set position of item APP_FILE_NAME to {160, 225}
			set position of item "Applications" to {550, 225}

			close
			open
			update without registering applications
		end tell
	end tell
end run
