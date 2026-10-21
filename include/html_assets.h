// Auto-generated from assets/index.html — edit the source, not this file.
#pragma once

static const char INDEX_HTML[] = R"HTML(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Garden PLC</title>
    <style>
        *,
        *::before,
        *::after {
            box-sizing: border-box;
            margin: 0;
            padding: 0
        }

        :root {
            --bg: #0c0c0c;
            --surface: #141414;
            --border: #222;
            --text: #d4d4d4;
            --dim: #555;
            --dim2: #3a3a3a;
            --on: #22c55e;
            --off: #444;
            --warn: #ef4444;
            --accent: #3b82f6;
        }

        body {
            background: var(--bg);
            color: var(--text);
            font-family: system-ui, -apple-system, sans-serif;
            font-size: 13px;
            padding: 14px;
            max-width: 900px;
            margin: 0 auto
        }

        a {
            color: var(--accent);
            text-decoration: none
        }

        /* ── header ── */
        header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding-bottom: 10px;
            border-bottom: 1px solid var(--border);
            margin-bottom: 14px
        }

        .site-name {
            font-size: 14px;
            font-weight: 700;
            letter-spacing: .3px
        }

        .site-name span {
            color: var(--accent)
        }

        .meta {
            color: var(--dim);
            font-size: 11px;
            display: flex;
            align-items: center;
            gap: 10px
        }

        .dot {
            width: 7px;
            height: 7px;
            border-radius: 50%;
            background: var(--on);
            flex-shrink: 0;
            transition: background .4s
        }

        .dot.stale {
            background: var(--warn)
        }

        /* ── section title ── */
        .sec {
            font-size: 9px;
            font-weight: 700;
            letter-spacing: 2.5px;
            text-transform: uppercase;
            color: var(--dim);
            margin: 16px 0 7px
        }

        /* ── grid ── */
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
            gap: 6px
        }

        /* ── card ── */
        .card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 5px;
            padding: 9px 11px
        }

        .card-active {
            border-color: var(--on)
        }

        .card-lbl {
            font-size: 10px;
            color: var(--dim);
            margin-bottom: 5px;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis
        }

        .card-val {
            font-size: 22px;
            font-weight: 800;
            line-height: 1;
            letter-spacing: -.5px
        }

        .card-sub {
            font-size: 10px;
            color: var(--dim);
            margin-top: 4px
        }

        /* ── state colours ── */
        .c-on {
            color: var(--on)
        }

        .c-off {
            color: var(--dim2)
        }

        .c-accent {
            color: var(--accent)
        }

        .c-warn {
            color: var(--warn)
        }

        /* ── badges ── */
        .badge {
            font-size: 10px;
            font-weight: 700;
            letter-spacing: .8px;
            padding: 2px 6px;
            border-radius: 3px
        }

        .badge-on {
            background: rgba(34, 197, 94, .12);
            color: var(--on);
            border: 1px solid rgba(34, 197, 94, .25)
        }

        .badge-off {
            background: rgba(68, 68, 68, .2);
            color: var(--dim);
            border: 1px solid var(--dim2)
        }

        .badge-warn {
            background: rgba(239, 68, 68, .12);
            color: var(--warn);
            border: 1px solid rgba(239, 68, 68, .25)
        }

        /* ── unit ── */
        .unit {
            font-size: 12px;
            font-weight: 500;
            margin-left: 2px;
            color: var(--dim)
        }

        .refill-button {
            border: 1px solid var(--accent);
            border-radius: 4px;
            padding: 7px 10px;
            background: transparent;
            color: var(--text);
            font: inherit;
            font-weight: 700;
            cursor: pointer
        }

        .refill-button:disabled {
            border-color: var(--border);
            color: var(--dim);
            cursor: not-allowed
        }

        /* ── footer ── */
        footer {
            margin-top: 18px;
            padding-top: 10px;
            border-top: 1px solid var(--border);
            display: flex;
            justify-content: space-between;
            align-items: center
        }

        .uptime {
            color: var(--dim);
            font-size: 10px
        }
    </style>
</head>

<body>
    <header>
        <div class="site-name">&#9881; Garden PLC <span>· Cabinet 1</span></div>
        <div class="meta">
            <span class="dot" id="dot"></span>
            <span id="ip">connecting…</span>
            <span id="fw"></span>
        </div>
    </header>

    <div class="sec">Selector</div>
    <div class="grid" id="selector"></div>

    <div class="sec">Control</div>
    <div class="grid">
        <div class="card">
            <div class="card-lbl">Well tank refill</div>
            <button class="refill-button" id="refill" type="button" onclick="requestRefill()">Trigger refill</button>
            <div class="card-sub" id="refill-status">Waiting for PLC</div>
        </div>
    </div>

    <div class="sec">Outputs</div>
    <div class="grid" id="outputs"></div>

    <div class="sec">Analog</div>
    <div class="grid" id="analog"></div>

    <footer>
        <a href="/update">&#8593; firmware update</a>
        <span class="uptime" id="uptime"></span>
    </footer>

    <script>
        var t0 = Date.now();

        function card(lbl, valHtml, sub, cls, cardCls) {
            return '<div class="card ' + (cardCls || '') + '"><div class="card-lbl">' + lbl + '</div>'
                + '<div class="card-val ' + (cls || '') + '">' + valHtml + '</div>'
                + (sub ? '<div class="card-sub">' + sub + '</div>' : '')
                + '</div>';
        }

        function badge(on) {
            return on
                ? '<span class="badge badge-on">ON</span>'
                : '<span class="badge badge-off">OFF</span>';
        }

        async function requestRefill() {
            var button = document.getElementById('refill');
            var status = document.getElementById('refill-status');
            button.disabled = true;
            status.textContent = 'Requesting refill';
            try {
                var response = await fetch('/api/control', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'name=refill&state=1'
                });
                var result = await response.json();
                if (!response.ok) throw new Error(result.error || 'Request rejected');
                status.textContent = 'Refill requested';
                refresh();
            } catch (error) {
                status.textContent = error.message;
                refresh();
            }
        }

        async function refresh() {
            try {
                var r = await fetch('/api/state');
                var d = await r.json();
                document.getElementById('dot').className = 'dot';
                document.getElementById('ip').textContent = d.ip;
                document.getElementById('fw').textContent = 'v' + d.firmware.replace('v', '');

                var modes = { '-1': 'INVALID', '0': 'OFF', '1': 'AUTOMATIC', '2': 'MANUAL' };
                var modeName = modes[String(d.switch_mode)] || 'INVALID';
                var modeClass = d.switch_mode === -1 ? 'c-warn' : d.switch_mode === 0 ? 'c-off' : 'c-on';
                document.getElementById('selector').innerHTML = card('Physical selector', modeName, null, modeClass);

                var refillButton = document.getElementById('refill');
                refillButton.disabled = d.switch_mode !== 1 || d.refill_active || d.refill_button;
                document.getElementById('refill-status').textContent = d.refill_active
                    ? 'Filling to high-level threshold'
                    : d.switch_mode !== 1
                        ? 'Automatic mode required'
                        : d.refill_button
                            ? 'Request pulse active'
                            : 'Ready';

                document.getElementById('outputs').innerHTML = d.outputs.map(function (x) {
                    return card(x.label, badge(x.state), null, x.state ? 'c-on' : 'c-off');
                }).join('');

                document.getElementById('analog').innerHTML = d.analog.map(function (x) {
                    if (!x.signal) return card(x.label, '<span class="badge badge-off">—</span>', null, 'c-off');
                    if (x.bar != null) {
                        return card(x.label,
                            x.bar.toFixed(2) + '<span class="unit">bar</span>',
                            x.mv + ' mV',
                            x.bar >= 4 ? 'c-on' : x.bar < 0.5 ? 'c-warn' : 'c-accent');
                    }
                    if (x.liters != null) {
                        return card(x.label,
                            x.liters.toFixed(0) + '<span class="unit">L</span>',
                            x.mv + ' mV',
                            x.low ? 'c-warn' : 'c-accent');
                    }
                    return card(x.label, x.mv + '<span class="unit">mV</span>', null, 'c-accent');
                }).join('');

                var s = ((Date.now() - t0) / 1000) | 0;
                document.getElementById('uptime').textContent =
                    (s >= 3600 ? (s / 3600 | 0) + 'h ' : '') +
                    (s >= 60 ? ((s % 3600) / 60 | 0) + 'm ' : '') +
                    (s % 60) + 's uptime';

            } catch (e) {
                document.getElementById('dot').className = 'dot stale';
            }
        }
        setInterval(refresh, 2000);
        refresh();
    </script>
</body>

</html>
)HTML";
