# ShiftSwap

Petit utilitaire Windows qui rétablit un comportement disparu de Windows 10 : **appuyer sur Shift désactive Verr Maj**.

*Tiny Windows utility that restores a feature missing from Windows 10: pressing Shift turns Caps Lock off.*

## Fonctionnement

- Dès que vous appuyez sur **Shift** alors que Verr Maj est actif, il est désactivé : les minuscules reprennent la priorité.
- **Shift + touche** donne une majuscule momentanée, puis retour en minuscule au relâchement.
- Pas de fenêtre ni de console : une icône apparaît dans la zone de notification.
- **Clic droit sur l'icône** : case « Lancer au demarrage » (démarrage automatique avec Windows, sans droits administrateur) et « Quitter ».

Le programme utilise un hook clavier global (`SetWindowsHookEx`) et n'envoie rien sur le réseau.

## Utilisation

Téléchargez `ShiftSwap.exe` depuis la page [Releases](../../releases) et lancez-le. Pour qu'il démarre avec Windows, cochez « Lancer au demarrage » dans le menu de l'icône.

## Compilation

Nécessite [MinGW-w64 / TDM-GCC](https://jmeubank.github.io/tdm-gcc/) (`gcc` et `windres` dans le PATH) :

```
windres ShiftSwap.rc -O coff -o ShiftSwap_res.o
gcc -mwindows -O2 ShiftSwap.c ShiftSwap_res.o -o ShiftSwap.exe -luser32 -lshell32 -ladvapi32
```

## Fichiers

| Fichier | Rôle |
|---|---|
| `ShiftSwap.c` | Code source |
| `ShiftSwap.rc` | Ressources (icône) |
| `ShiftSwap.ico` | Icône |

## Licence

[MIT](LICENSE)
