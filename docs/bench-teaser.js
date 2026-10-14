/* The landing page's "Closest to Chromium" benchmark teaser, drawn in the browser.
 *
 * It fetches benchmarks/render_benchmark.json (published next to the benchmark report), averages the
 * raw per-sample numbers itself and draws the two bar charts. Nothing about the numbers is baked into
 * the page at docs-build time. If the file is missing, unreadable or has no novasvg in it, the section
 * simply stays hidden: the page is the same as if the teaser did not exist.
 *
 * generate_index.py inlines this file into the page; the section's markup comes from its bench_shell().
 */
(function () {
    'use strict';

    var section = document.getElementById('benchmarks');
    if (!section) return;

    var REFERENCE = 'ground_truth';   // the Chromium render everything is scored against, not a contestant
    var NOVA = 'novasvg';
    // same colours as benchmarks/report/charts.py; novasvg itself is drawn with the logo gradient (.bar.nova)
    var COLORS = { resvg: '#3f8fe0', lunasvg: '#6f7fe0', cairosvg: '#9a6fd6', thorvg: '#c565c4', nanosvg: '#e8739f' };

    function mean(xs) {
        if (!xs.length) return null;
        var sum = 0;
        for (var i = 0; i < xs.length; i++) sum += xs[i];
        return sum / xs.length;
    }

    // {engine key: {label, version, avgMs, rmse}}, averaged from the per-sample cells of the contestants.
    // Same rule as benchmarks/report/gather.py: time over the samples that rendered, RMSE over those that have one.
    function summarise(data) {
        var meta = data.summary || {};
        var engines = {};
        Object.keys(data.matrix).forEach(function (name) {
            var row = data.matrix[name] || {};
            Object.keys(row).forEach(function (key) {
                if (key === REFERENCE) return;
                var e = engines[key] || (engines[key] = { times: [], errs: [] });
                var cell = row[key];
                if (!cell || !cell.ok || typeof cell.seconds !== 'number') return;
                e.times.push(cell.seconds * 1000);
                if (typeof cell.rmse === 'number') e.errs.push(cell.rmse);
            });
        });
        var out = {};
        Object.keys(engines).forEach(function (key) {
            var m = meta[key] || {};
            out[key] = {
                label: m.label || key,
                version: String(m.version || '').split(' (')[0],
                avgMs: mean(engines[key].times),
                rmse: mean(engines[key].errs)
            };
        });
        return out;
    }

    function el(tag, cls, text) {
        var node = document.createElement(tag);
        if (cls) node.className = cls;
        if (text !== undefined) node.textContent = text;
        return node;
    }

    // rows: [[key, label, value]] -> <ul class="bars">, shortest first; same markup as charts.py's _bars()
    function bars(rows, unit, digits, aria) {
        rows = rows.filter(function (r) { return r[2] !== null; }).sort(function (a, b) { return a[2] - b[2]; });
        if (!rows.length) return null;
        var top = Math.max.apply(null, rows.map(function (r) { return r[2]; })) || 1;
        var ul = el('ul', 'bars');
        ul.setAttribute('role', 'img');
        ul.setAttribute('aria-label', aria);
        rows.forEach(function (r, i) {
            var isNova = r[0] === NOVA;
            var li = el('li', 'bar-row' + (isNova ? ' is-nova' : ''));
            var bar = el('span', isNova ? 'bar nova' : 'bar');
            bar.style.setProperty('--w', Math.max(r[2] / top * 100, 1.5).toFixed(1) + '%');
            bar.style.setProperty('--i', i);
            if (!isNova) bar.style.setProperty('--c', COLORS[r[0]] || 'var(--accent)');
            var track = el('span', 'bar-track');
            track.appendChild(bar);
            var value = el('span', 'bar-value', r[2].toFixed(digits));
            value.appendChild(el('small', '', unit));
            li.appendChild(el('span', 'bar-label', r[1]));
            li.appendChild(track);
            li.appendChild(value);
            ul.appendChild(li);
        });
        return ul;
    }

    function fill(selector, node) {
        var slot = section.querySelector(selector);
        if (slot && node) slot.appendChild(node);
    }

    function render(data) {
        var engines = summarise(data);
        if (!engines[NOVA]) return false;
        var keys = Object.keys(engines);
        var withErr = keys.filter(function (k) { return engines[k].rmse !== null; })
                          .sort(function (a, b) { return engines[a].rmse - engines[b].rmse; });
        var withTime = keys.filter(function (k) { return engines[k].avgMs !== null; })
                           .sort(function (a, b) { return engines[a].avgMs - engines[b].avgMs; });
        var closest = withErr.length > 0 && withErr[0] === NOVA;
        var fastest = withTime.length > 0 && withTime[0] === NOVA;
        var n = (data.files && data.files.length) || 'all';

        section.querySelector('h2').textContent = closest ? 'Closest to Chromium' : 'How it compares';
        section.querySelector('.lede').textContent =
            (closest ? 'Across ' + n + " sample SVGs, NovaSVG's output has the lowest mean error against Chromium. "
                     : 'Across ' + n + ' sample SVGs, each renderer is compared with Chromium on accuracy and speed. ') +
            (fastest ? 'It is also the fastest.'
                     : 'It is not the fastest renderer, and the full report shows where it wins and where it loses.');

        fill('[data-chart="accuracy"]', bars(keys.map(function (k) { return [k, engines[k].label, engines[k].rmse]; }),
            '', 1, 'Mean RMSE against Chromium, lower is closer to the reference'));
        fill('[data-chart="speed"]', bars(keys.map(function (k) { return [k, engines[k].label, engines[k].avgMs]; }),
            ' ms', 2, 'Average render time per sample in milliseconds, lower is faster'));

        var versions = keys.map(function (k) { return engines[k].label + ' ' + engines[k].version; }).join(', ');
        section.querySelector('.bench-foot small').textContent = n + ' samples, median of ' + (data.runs || '?') + ' runs' +
            (data.generated ? ', generated ' + data.generated : '') + '. Measured with ' + versions + '.';
        return true;
    }

    // grow the bars once they scroll into view
    function animate() {
        var charts = section.querySelectorAll('.bars');
        if (!('IntersectionObserver' in window)) {
            charts.forEach(function (c) { c.classList.add('in'); });
            return;
        }
        var io = new IntersectionObserver(function (entries) {
            entries.forEach(function (e) {
                if (e.isIntersecting) { e.target.classList.add('in'); io.unobserve(e.target); }
            });
        }, { threshold: 0.25 });
        charts.forEach(function (c) { io.observe(c); });
    }

    fetch(section.getAttribute('data-src'))
        .then(function (res) {
            if (!res.ok) throw new Error('HTTP ' + res.status);
            return res.json();
        })
        .then(function (data) {
            if (!data || typeof data.matrix !== 'object' || !data.matrix || !render(data)) return;
            section.hidden = false;
            animate();
        })
        .catch(function () { /* no usable benchmark data: leave the section hidden */ });
})();
