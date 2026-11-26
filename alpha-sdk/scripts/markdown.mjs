// The small Markdown subset the docs use: # headings, ``` fences, - lists, | tables,
// paragraphs; inline `code`, **bold**, [links](x.md) (.md -> .html for pages of this site; absolute
// URLs are left as written). Nothing else.
export const escape = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function inline(text) {
  return escape(text).split(/(`[^`]+`)/).map((part) => {
    if (part.startsWith("`") && part.endsWith("`") && part.length > 1) return `<code>${part.slice(1, -1)}</code>`;
    return part.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
      .replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (_, label, href) => `<a href="${/^[a-z]+:/i.test(href) ? href : href.replace(/\.md(#|$)/, ".html$1")}">${label}</a>`);
  }).join("");
}

const slug = (s) => s.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "");
const cells = (row) => row.trim().replace(/^\||\|$/g, "").split("|").map((c) => c.trim());

function table(lines) {
  const [head, , ...rows] = lines;
  return `<table><thead><tr>${cells(head).map((c) => `<th>${inline(c)}</th>`).join("")}</tr></thead><tbody>${
    rows.map((r) => `<tr>${cells(r).map((c) => `<td>${inline(c)}</td>`).join("")}</tr>`).join("")}</tbody></table>`;
}

// Consumes the block starting at lines[i]; returns [html, next index].
function block(lines, i) {
  const line = lines[i];
  if (line.startsWith("```")) {
    const end = lines.indexOf("```", i + 1) < 0 ? lines.length : lines.indexOf("```", i + 1);
    return [`<pre><code>${escape(lines.slice(i + 1, end).join("\n"))}</code></pre>`, end + 1];
  }
  const h = /^(#{1,4}) (.*)$/.exec(line);
  if (h) return [`<h${h[1].length} id="${slug(h[2])}">${inline(h[2])}</h${h[1].length}>`, i + 1];
  const run = (test) => {
    let j = i;
    while (j < lines.length && test(lines[j])) j++;
    return j;
  };
  if (line.startsWith("|")) {
    const j = run((l) => l.startsWith("|"));
    return [table(lines.slice(i, j)), j];
  }
  if (/^- /.test(line)) {
    const j = run((l) => /^- /.test(l) || /^ {2}\S/.test(l));
    const items = lines.slice(i, j).join("\n").split(/\n(?=- )/).map((it) => `<li>${inline(it.slice(2).replace(/\n\s+/g, " "))}</li>`);
    return [`<ul>${items.join("")}</ul>`, j];
  }
  const j = run((l) => l.trim() && !/^(#{1,4} |```|- |\|)/.test(l));
  return [`<p>${inline(lines.slice(i, Math.max(j, i + 1)).join(" "))}</p>`, Math.max(j, i + 1)];
}

/** HTML of a Markdown text; `bare` drops the <p> around a single paragraph (table cells). */
export function markdown(text, bare = false) {
  const lines = text.replace(/\r\n/g, "\n").split("\n");
  const out = [];
  for (let i = 0; i < lines.length;) {
    if (!lines[i].trim()) {
      i++;
      continue;
    }
    const [html, next] = block(lines, i);
    out.push(html);
    i = next;
  }
  const html = out.join("\n");
  return bare && out.length === 1 && html.startsWith("<p>") ? html.slice(3, -4) : html;
}
