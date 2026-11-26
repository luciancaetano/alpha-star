# Real example captures

Captured on Windows on 2026-11-25 from commit 2525239, using the staged 0.2.0
release engine copied into the isolated workcopy. No generated game imagery.
The shaders example is deliberately excluded while it is under development.

Install SDK and each example with npm. Set ALPHA_STAR to the absolute path of
the release engine, then run from the repository root (PowerShell):

```powershell
$root = (Get-Location).Path
python tools/resource_guard.py -- npm.cmd --prefix examples/platformer start -- -- --frames 45 --screenshot "$root/docs/guide/assets/screenshot-platformer.png"
foreach ($example in @('flappy', 'puzzle')) {
  python tools/resource_guard.py -- npm.cmd --prefix "examples/$example" start -- -- --frames 90 --screenshot "$root/docs/guide/assets/screenshot-$example.png"
}
foreach ($example in @('topdown', 'shmup')) {
  python tools/resource_guard.py -- npm.cmd --prefix "examples/$example" start -- -- --frames 300 --tap Space --tap-every 12 --screenshot "$root/docs/guide/assets/screenshot-$example.png"
}
npm.cmd --prefix alpha-sdk run docs
```

The engine opens each game normally, captures its rendered frame, and exits.
Persistent game preferences can affect locale and the selected puzzle level.
Keep full frames at native dimensions; the landing handles portrait and landscape
images without cropping HUDs. Asset licenses remain in the respective examples.
