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
