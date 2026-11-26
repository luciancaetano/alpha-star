import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

function element() {
  return { hidden: true, attributes: {}, events: {},
    setAttribute(name, value) { this.attributes[name] = value; },
    addEventListener(name, handler) { this.events[name] = handler; },
    emit(name, event = {}) { this.events[name](event); },
  };
}
function setup() {
  const slides = Array.from({ length: 6 }, element);
  slides[0].hidden = false;
  const buttons = Array.from({ length: 6 }, element);
  const controls = element(), previous = element(), next = element();
  const slider = Object.assign(element(), {
    querySelectorAll: (selector) => selector === '[data-slide]' ? slides : buttons,
    querySelector: (selector) => ({ '.slider-controls': controls, '[data-slider-prev]': previous, '[data-slider-next]': next })[selector],
  });
  vm.runInNewContext(fs.readFileSync(new URL('../scripts/landing.js', import.meta.url), 'utf8'), {
    document: { querySelector: (selector) => selector === '#example-slider' ? slider : null },
  });
  const active = () => slides.findIndex((slide) => !slide.hidden);
  return { slides, buttons, controls, previous, next, slider, active };
}

test('slider cycles both ways, selects every example and exposes one slide', () => {
  const s = setup();
  assert.equal(s.controls.hidden, false);
  assert.equal(s.active(), 0);
  s.previous.emit('click');
  assert.equal(s.active(), 5);
  s.next.emit('click');
  assert.equal(s.active(), 0);
  s.buttons.forEach((button, index) => {
    button.emit('click');
    assert.equal(s.active(), index);
    assert.equal(s.slides.filter((slide) => !slide.hidden).length, 1);
    assert.equal(button.attributes['aria-pressed'], 'true');
    assert.equal(s.buttons.filter((b) => b.attributes['aria-pressed'] === 'true').length, 1);
  });
});

test('slider supports keyboard and swipe without hijacking Tab or small gestures', () => {
  const s = setup();
  let prevented = 0;
  const key = (value) => s.slider.emit('keydown', { key: value, preventDefault() { prevented++; } });
  key('End'); assert.equal(s.active(), 5);
  key('ArrowRight'); assert.equal(s.active(), 0);
  key('ArrowLeft'); assert.equal(s.active(), 5);
  key('Home'); assert.equal(s.active(), 0);
  key('Tab'); assert.equal(prevented, 4);
  const swipe = (start, end) => {
    s.slider.emit('touchstart', { touches: [{ clientX: start }] });
    s.slider.emit('touchend', { changedTouches: [{ clientX: end }] });
  };
  swipe(200, 100); assert.equal(s.active(), 1);
  swipe(100, 200); assert.equal(s.active(), 0);
  swipe(100, 110); assert.equal(s.active(), 0);
});
