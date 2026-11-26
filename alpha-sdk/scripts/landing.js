const copyButton = document.querySelector("#copy-command");
if (copyButton && navigator.clipboard?.writeText) {
  copyButton.hidden = false;
  copyButton.addEventListener("click", async () => {
    const status = document.querySelector("#copy-status");
    try {
      await navigator.clipboard.writeText(document.querySelector("#start-commands").textContent);
      status.textContent = "Commands copied. Paste them into your terminal.";
    } catch {
      status.textContent = "Select the commands above and copy them manually.";
    }
  });
}

const slider = document.querySelector("#example-slider");
if (slider) {
  const slides = [...slider.querySelectorAll("[data-slide]")];
  const selectors = [...slider.querySelectorAll("[data-select-slide]")];
  let active = 0;
  const show = (index) => {
    active = (index + slides.length) % slides.length;
    slides.forEach((slide, i) => { slide.hidden = i !== active; });
    selectors.forEach((button, i) => button.setAttribute("aria-pressed", String(i === active)));
  };
  slider.querySelector(".slider-controls").hidden = false;
  slider.querySelector("[data-slider-prev]").addEventListener("click", () => show(active - 1));
  slider.querySelector("[data-slider-next]").addEventListener("click", () => show(active + 1));
  selectors.forEach((button, i) => button.addEventListener("click", () => show(i)));
  slider.addEventListener("keydown", (event) => {
    const destination = { ArrowLeft: active - 1, ArrowRight: active + 1, Home: 0, End: slides.length - 1 }[event.key];
    if (destination === undefined) return;
    event.preventDefault();
    show(destination);
  });
  let touchX;
  slider.addEventListener("touchstart", (event) => { touchX = event.touches[0].clientX; }, { passive: true });
  slider.addEventListener("touchend", (event) => {
    const delta = event.changedTouches[0].clientX - touchX;
    if (Math.abs(delta) > 50) show(active + (delta < 0 ? 1 : -1));
    touchX = undefined;
  }, { passive: true });
}
