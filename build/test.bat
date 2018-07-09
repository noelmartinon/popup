start popup.exe -t"Information" -m"Hello world !"

start popup.exe -t"Warning" -m"Option '-l' is used:.\nThe next popup windows will not be shown until that one is displayed" -i2 -l

start popup.exe -t"Custom icon & color + long msg" -m"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. \nSuspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor. \nCras elementum ultrices diam." -i"..\images\config.png" -bff0fa0 -d4

start popup.exe -t"Custom background image" -m"Lorem ipsum dolor" -i"..\images\Apps-nepomuk-icon.png" -b"..\images\Popup_bubble.png"