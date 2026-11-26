const REPO = "https://github.com/luciancaetano/alpha-star";

const GAMES = [
  ["shaders", "Give every frame a new look.", "Shader Lab", "Six custom GLSL effects, orbiting lights and adjustable intensity.", "GLSL / Lighting"],
  ["platformer", "A little world. A big jump.", "Platformer", "Sprite animation, LDtk levels, camera follow and Box2D physics.", "Physics / Tilemaps"],
  ["topdown", "Bring a friend. Clear the arena.", "Top-down arena", "Two players, auto-aim, sensor bullets and particle blasts.", "Co-op / Particles"],
  ["shmup", "Find your way through the fire.", "Space shooter", "Fast shots, collision categories and a scrolling star field.", "Collision / Layers"],
  ["puzzle", "One more move. One more try.", "Sokoban puzzle", "Pure game rules, undo, saved progress and accessible UI.", "Saves / Game UI"],
  ["flappy", "Small game. Complete experience.", "Flappy", "Scenes, music, settings, localization and a best score to beat.", "Audio / Localization"],
];

function gameLink(id) {
  return id === "shaders"
    ? `<a href="screenshot-shaders.png">View full capture</a>`
    : `<a href="${REPO}/tree/main/examples/${id}">Explore source</a>`;
}

// The first slide: a muted, looping engine capture of examples/survivors' showcase run
// (recorded with examples/survivors/tools/record-showcase.mjs, encoded by encode-showcase.py).
const SHOWCASE = ["survivors", "Candlewick", "50 powers, hundreds of foes, a screen full of projectiles: one evolved build in real time."];

function showcaseSlide() {
  const [id, name, description] = SHOWCASE;
  return `<figure class="hero-slide" data-slide="${id}" role="group" aria-roledescription="slide" aria-label="1 of ${GAMES.length + 1}: ${name}">
    <div class="screen-bar"><span>${name} <span class="muted">/ alpha-star</span></span><span class="screen-caption">REAL ENGINE CAPTURE</span></div>
    <video src="showcase-${id}.mp4" poster="showcase-${id}.png" autoplay muted loop playsinline preload="auto" width="768" height="432" aria-label="${name} running in alpha-star, no sound"></video>
    <figcaption><span>${description}</span><a href="${REPO}/tree/main/examples/${id}">Explore source</a></figcaption></figure>`;
}

function heroSlider() {
  const total = GAMES.length + 1;
  const slides = [showcaseSlide(), ...GAMES.map(([id, , name, description], index) => `<figure class="hero-slide" data-slide="${id}" role="group" aria-roledescription="slide" aria-label="${index + 2} of ${total}: ${name}" hidden>
    <div class="screen-bar"><span>${name} <span class="muted">/ alpha-star</span></span><span class="screen-caption">REAL ENGINE CAPTURE</span></div>
    <img src="screenshot-${id}.png" alt="${name} running in alpha-star" loading="lazy" width="640" height="360">
    <figcaption><span>${description}</span>${gameLink(id)}</figcaption></figure>`)].join("\n");
  const names = [SHOWCASE[1], ...GAMES.map(([, , name]) => name)];
  const buttons = names.map((name, index) => `<button type="button" data-select-slide="${index}" aria-label="Show ${name}" aria-pressed="${index === 0}">${name}</button>`).join("");
  return `<section id="example-slider" class="hero-screen" role="region" aria-roledescription="carousel" aria-label="Example games" tabindex="0">
    <div class="slides" aria-live="polite">${slides}</div>
    <div class="slider-controls" hidden><button type="button" data-slider-prev aria-label="Previous example">Previous</button><div class="slide-selectors">${buttons}</div><button type="button" data-slider-next aria-label="Next example">Next</button></div>
  </section>`;
}

function gameCards() {
  return GAMES.map(([id, headline, name, description, tags]) => `<article class="game-card game-${id}">
    <a class="game-image" href="screenshot-${id}.png" aria-label="View full screenshot of ${name}"><img src="screenshot-${id}.png" alt="${name} running in the alpha-star engine" loading="lazy" decoding="async"><span class="image-label">${tags}</span></a>
    <div class="game-copy"><p class="eyebrow">${name}</p><h3>${headline}</h3><p>${description}</p><span class="text-link">${gameLink(id)} <span aria-hidden="true">+</span></span></div>
  </article>`).join("\n");
}

/** The public home page. Documentation remains in its own generated pages. */
export function landing() {
  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>alpha-star — Small engine. Playable ideas.</title>
  <meta name="description" content="An open-source 2D game engine for AI agents and the people building with them. Write typed game code, test it, and play. Explore six real engine examples.">
  <meta name="theme-color" content="#10120f"><meta property="og:title" content="alpha-star — Small engine. Playable ideas.">
  <meta property="og:description" content="A native 2D engine. A typed scripting API. A shorter path from an idea to a game you can play.">
  <meta property="og:type" content="website"><meta property="og:image" content="https://luciancaetano.github.io/alpha-star/screenshot-platformer.png">
  <link rel="icon" href="aster.png" type="image/png"><link rel="stylesheet" href="landing.css">
  <script src="landing.js" defer></script>
</head>
<body>
<a class="skip-link" href="#main">Skip to content</a>
<header class="site-header wrap">
  <a class="brand" href="index.html" aria-label="alpha-star home"><img class="header-logo" src="logo.png" width="160" height="107" alt="alpha-star engine"></a>
  <nav aria-label="Main navigation"><a href="#examples">Examples</a><a href="getting-started.html">Docs</a><a class="github-link" href="${REPO}">GitHub <svg viewBox="0 0 24 24" width="19" height="19" aria-hidden="true"><path fill="currentColor" d="M12 2a10 10 0 0 0-3.16 19.49c.5.09.68-.22.68-.48v-1.86c-2.78.6-3.37-1.18-3.37-1.18-.45-1.16-1.11-1.47-1.11-1.47-.91-.62.07-.61.07-.61 1 .07 1.53 1.03 1.53 1.03.89 1.53 2.34 1.09 2.91.83.09-.65.35-1.09.64-1.34-2.22-.25-4.56-1.11-4.56-4.94 0-1.09.39-1.99 1.03-2.69-.1-.25-.45-1.27.1-2.65 0 0 .84-.27 2.75 1.03A9.6 9.6 0 0 1 12 6.82c.85 0 1.71.11 2.51.34 1.91-1.3 2.75-1.03 2.75-1.03.55 1.38.2 2.4.1 2.65.64.7 1.03 1.6 1.03 2.69 0 3.84-2.34 4.69-4.57 4.94.36.31.68.92.68 1.85v2.75c0 .27.18.58.69.48A10 10 0 0 0 12 2Z"/></svg></a></nav>
</header>
<main id="main">
  <section class="hero wrap" aria-labelledby="hero-title">
    <div class="hero-heading"><p class="eyebrow"><span class="pixel-spark" aria-hidden="true"></span> An open-source 2D game engine</p>
      <h1 id="hero-title">Small engine.<br><span>Playable ideas.</span></h1>
      <div class="hero-intro"><p>For the games in your head.<br>A native engine and a typed API for AI agents and the people building with them.</p><div class="actions"><a class="button primary" href="#start">Make your first game</a><a class="button secondary" href="#examples">See it in action</a></div></div>
    </div>
    ${heroSlider()}
    <div class="engine-strip"><p>Small surface.<br><strong>Solid foundations.</strong></p><span>C++20</span><span>SDL3</span><span>Box2D</span><span>QuickJS</span><span>TypeScript</span></div>
  </section>

  <section class="built-for wrap section" aria-labelledby="built-title"><div class="section-heading"><p class="eyebrow">Made for the way you build</p><h2 id="built-title">Your idea.<br>Your agent. <span>Your game.</span></h2><p>Keep the creative loop close: write, run, inspect, repeat. A small, predictable API gives you and your agent the same clear starting point.</p></div>
    <div class="features"><article><span class="feature-symbol" aria-hidden="true">{ }</span><h3>Code with a clear contract.</h3><p>TypeScript checks your scenes, actions, tags and assets. Catch a misspelled name before it becomes a broken game.</p><a href="reference.html" class="text-link">Explore the API <span aria-hidden="true">+</span></a></article><article><span class="feature-symbol" aria-hidden="true">&gt;_</span><h3>Give your agent the controls.</h3><p>Through MCP, an agent can build, run and inspect the game. Hot reload keeps the feedback loop moving.</p><a href="mcp.html" class="text-link">Meet the agent workflow <span aria-hidden="true">+</span></a></article><article><span class="feature-symbol" aria-hidden="true">[=]</span><h3>Make play reproducible.</h3><p>A fixed step, seeded randomness and headless tests let you check game behavior with repeatable inputs.</p><a href="testing.html" class="text-link">See how testing works <span aria-hidden="true">+</span></a></article></div>
  </section>

  <section id="examples" class="examples-section section" aria-labelledby="examples-title"><div class="wrap"><div class="section-heading split-heading"><div><p class="eyebrow">Built with alpha-star</p><h2 id="examples-title">Less imagining.<br><span>More playing.</span></h2></div><p>Five small games and a shader playground. Real screenshots from the running engine. Read the code, learn a pattern, make it yours.</p></div><div class="games-grid">${gameCards()}<aside class="gallery-note"><span class="pixel-spark large" aria-hidden="true"></span><h3>A starting point,<br>not a blank page.</h3><p>Each example includes tests and an AGENTS.md. Platformer, top-down, puzzle and shmup are also ready-to-use templates.</p><a class="text-link" href="examples.html">Read the example guide <span aria-hidden="true">+</span></a></aside></div></div></section>

  <section id="start" class="start-section wrap section" aria-labelledby="start-title"><div class="section-heading"><p class="eyebrow">From terminal to playtime</p><h2 id="start-title">Start small.<br><span>Make it yours.</span></h2><p>Pick a template. Install its dependencies. Run the tests, then take the controls.</p><a class="button secondary" href="getting-started.html">Read the getting started guide</a></div><div class="terminal"><div class="terminal-bar"><span>Terminal</span><button type="button" id="copy-command" hidden>Copy commands</button></div><pre><code id="start-commands">npx @alpha-star/sdk init my-game --template platformer
cd my-game
npm install
npm test
npm start</code></pre><p class="terminal-note">Other templates: blank · topdown · puzzle · shmup</p><p id="copy-status" class="copy-status" role="status" aria-live="polite"></p></div></section>

  <section class="closing wrap"><img src="aster.png" alt="Aster, the alpha-star mascot" width="88" height="88" loading="lazy"><div><h2>There’s a game in there.</h2><p>Give it somewhere to run.</p></div><a class="button primary" href="getting-started.html">Build with alpha-star</a></section>
</main>
<footer class="site-footer wrap"><a class="brand" href="index.html">alpha<span class="brand-star">*</span>star</a><p>A small engine for big little ideas.</p><nav aria-label="Footer"><a href="getting-started.html">Documentation</a><a href="${REPO}">Source code</a><a href="${REPO}/blob/main/LICENSE">MIT license</a></nav></footer>
</body></html>
`;
}
