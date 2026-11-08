MOAI GLITCH 1.1.0 para macOS  -  by Moaiact
VST3 + AU (Logic Pro, Ableton Live, Reaper, FL Studio, Cubase, etc.)
Compatible con Mac Apple Silicon (M1/M2/M3/M4) e Intel. macOS 11 o superior.

============================  ESPAÑOL  ============================

INSTALACIÓN
1. Haz doble clic en "Moai Glitch 1.1.0 (macOS).pkg".
2. macOS te mostrará un aviso porque el plugin es gratuito y no está
   firmado con una cuenta de pago de Apple. Es normal. Para abrirlo:
     - Cierra el aviso (botón "Aceptar" / "Listo").
     - Abre  Ajustes del Sistema  ->  Privacidad y seguridad.
     - Baja hasta el final: verás "Se bloqueó Moai Glitch...".
       Haz clic en  "Abrir igualmente"  y confirma con tu contraseña.
3. Sigue el instalador. Los plugins quedan en:
     /Library/Audio/Plug-Ins/VST3/Moai Glitch.vst3
     /Library/Audio/Plug-Ins/Components/Moai Glitch.component
4. Abre tu DAW y haz un rescan de plugins. Lo encontrarás como
   "Moai Glitch", fabricante "Moaiact".

SI TU DAW NO LO MUESTRA O DICE QUE ESTÁ DAÑADO
Abre la app  Terminal  y pega estas dos líneas (te pedirá tu contraseña):
  sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Moai Glitch.vst3"
  sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/Moai Glitch.component"
Luego reinicia el DAW. En Logic Pro, si no aparece, reinicia el Mac.

============================  ENGLISH  ============================

INSTALLATION
1. Double-click "Moai Glitch 1.1.0 (macOS).pkg".
2. macOS will show a warning because this free plugin is not signed with
   a paid Apple developer account. This is normal. To open it:
     - Close the warning ("OK" / "Done").
     - Open  System Settings  ->  Privacy & Security.
     - Scroll to the bottom: you'll see "Moai Glitch... was blocked".
       Click  "Open Anyway"  and confirm with your password.
3. Follow the installer. The plugins are installed to:
     /Library/Audio/Plug-Ins/VST3/Moai Glitch.vst3
     /Library/Audio/Plug-Ins/Components/Moai Glitch.component
4. Open your DAW and rescan plugins. Look for "Moai Glitch" by "Moaiact".

IF YOUR DAW DOESN'T SHOW IT OR SAYS IT IS DAMAGED
Open the  Terminal  app and paste these two lines (it will ask for your password):
  sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Moai Glitch.vst3"
  sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/Moai Glitch.component"
Then restart your DAW. In Logic Pro, restart the Mac if it still doesn't appear.

Free plugin. Do not resell.  (c) 2026 Moaiact.
