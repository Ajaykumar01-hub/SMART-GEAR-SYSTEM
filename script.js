/**
 * SMART GEAR OPTIMIZATION SYSTEM — SCRIPT.JS
 * Connects frontend telemetry directly to C++ server on port 18080.
 */

const BACKEND_URL = "http://127.0.0.1:18080/api/optimize";

const VEHICLE_PRESETS = {
  hatchback: {
    name: "Compact Hatchback 1.2L",
    curbWeight: 1050,
    cd: 0.31,
    frontalArea: 2.15,
    wheelRadius: 0.305,
    finalDrive: 4.05,
    gearRatios: [3.55, 1.95, 1.30, 0.89, 0.71],
    rpmLug: 1200,
    rpmEcoMin: 1700,
    rpmEcoMax: 2400,
    rpmRedline: 6200,
    baseBSFC: 245
  },
  sedan: {
    name: "Midsize Sedan 2.0L Diesel",
    curbWeight: 1520,
    cd: 0.28,
    frontalArea: 2.25,
    wheelRadius: 0.325,
    finalDrive: 3.65,
    gearRatios: [3.82, 2.05, 1.35, 0.98, 0.76, 0.62],
    rpmLug: 1100,
    rpmEcoMin: 1400,
    rpmEcoMax: 2100,
    rpmRedline: 4800,
    baseBSFC: 210
  },
  motorcycle: {
    name: "Commuter Motorcycle 150cc",
    curbWeight: 140,
    cd: 0.60,
    frontalArea: 0.75,
    wheelRadius: 0.29,
    finalDrive: 3.10,
    gearRatios: [2.85, 1.82, 1.35, 1.05, 0.88],
    rpmLug: 2500,
    rpmEcoMin: 4000,
    rpmEcoMax: 6000,
    rpmRedline: 9500,
    baseBSFC: 290
  },
  truck: {
    name: "Commercial Cargo Truck 4.5T",
    curbWeight: 4200,
    cd: 0.68,
    frontalArea: 5.20,
    wheelRadius: 0.44,
    finalDrive: 4.10,
    gearRatios: [6.12, 3.45, 2.15, 1.42, 1.00, 0.78],
    rpmLug: 1000,
    rpmEcoMin: 1300,
    rpmEcoMax: 1800,
    rpmRedline: 2800,
    baseBSFC: 215
  }
};

const state = {
  activeVehicleKey: "hatchback",
  vehicle: JSON.parse(JSON.stringify(VEHICLE_PRESETS.hatchback)),
  speed: 60,
  engagedGear: 3,
  passengers: 1,
  cargoKg: 20,
  roadType: "highway",
  gradientPct: 0.0,
  scenarioHistory: []
};

// 1. Fetch Call to C++ Backend
async function fetchShiftRecommendation() {
  const payload = {
    vehicle: state.vehicle,
    speed: state.speed,
    engagedGear: state.engagedGear,
    passengers: state.passengers,
    cargoKg: state.cargoKg,
    gradientPct: state.gradientPct,
    roadType: state.roadType
  };

  const startTime = performance.now();

  try {
    const response = await fetch(BACKEND_URL, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    if (!response.ok) throw new Error("HTTP Status: " + response.status);

    const data = await response.json();
    const duration = (performance.now() - startTime).toFixed(1);

    // Update Pipe Inspector
    const latencyEl = document.getElementById("api-latency-badge");
    const reqEl = document.getElementById("inspector-request");
    const resEl = document.getElementById("inspector-response");
    const statusText = document.getElementById("system-status-text");

    if (latencyEl) latencyEl.textContent = `Latency: ${duration} ms (Native C++ Winsock)`;
    if (reqEl) reqEl.textContent = JSON.stringify(payload, null, 2);
    if (resEl) resEl.textContent = JSON.stringify(data, null, 2);
    if (statusText) statusText.textContent = "CONNECTED: C++ OOP ENGINE (PORT 18080)";

    return data;
  } catch (err) {
    const statusText = document.getElementById("system-status-text");
    if (statusText) statusText.textContent = "C++ OFFLINE — CLIENT FALLBACK";
    return fallbackLocalSimulation();
  }
}

// 2. Client-Side Fallback if C++ server isn't running
function fallbackLocalSimulation() {
  const speedMs = state.speed / 3.6;
  const wheelOmega = speedMs / state.vehicle.wheelRadius;
  const ratio = state.vehicle.gearRatios[state.engagedGear - 1];
  const rpm = Math.round((wheelOmega * ratio * state.vehicle.finalDrive * 60) / (2 * Math.PI));

  return {
    currentRpm: state.speed > 0 ? rpm : 800,
    currentL100km: 6.2,
    optimalGear: state.engagedGear,
    optimalL100km: 5.8,
    adviceType: "maintain",
    explanation: "C++ Backend offline. Displaying local client telemetry preview.",
    drag: { total: 340, aero: 160, rolling: 140, grade: 40 },
    gearEvaluations: state.vehicle.gearRatios.map((r, i) => ({
      gear: i + 1,
      rpm: Math.round((wheelOmega * r * state.vehicle.finalDrive * 60) / (2 * Math.PI)),
      fuelRate: 6.0,
      status: "feasible",
      suitability: "Acceptable"
    }))
  };
}

// 3. UI Synchronization
async function refreshDashboard() {
  const res = await fetchShiftRecommendation();

  document.getElementById("speed-display").textContent = state.speed;
  document.getElementById("rpm-display").textContent = res.currentRpm;
  document.getElementById("val-speed").textContent = `${state.speed} km/h`;
  document.getElementById("val-gear").textContent = `Gear ${state.engagedGear}`;
  document.getElementById("matrix-speed-badge").textContent = `${state.speed} km/h`;
  document.getElementById("active-vehicle-label").textContent = state.vehicle.name;

  document.getElementById("current-gear-num").textContent = state.engagedGear;
  document.getElementById("recommended-gear-num").textContent = res.optimalGear;

  const arrow = document.getElementById("shift-arrow");
  const expCard = document.getElementById("explanation-box");
  const expTitle = document.getElementById("explanation-title");
  const banner = document.getElementById("status-tag-banner");

  expCard.className = "explanation-card " + (res.adviceType === "upshift" ? "" : res.adviceType);

  if (res.adviceType === "upshift") {
    arrow.textContent = "▲";
    arrow.style.color = "var(--brand-cyan)";
    expTitle.innerHTML = "<span>▲</span> Recommendation: Shift Up";
    banner.textContent = "SHIFT UP";
    banner.className = "status-tag overrev";
  } else if (res.adviceType === "downshift") {
    arrow.textContent = "▼";
    arrow.style.color = "var(--brand-amber)";
    expTitle.innerHTML = "<span>▼</span> Recommendation: Shift Down";
    banner.textContent = "SHIFT DOWN";
    banner.className = "status-tag lugging";
  } else {
    arrow.textContent = "➔";
    arrow.style.color = "var(--brand-emerald)";
    expTitle.innerHTML = "<span>✔</span> Gear Optimized";
    banner.textContent = "OPTIMAL";
    banner.className = "status-tag optimal";
  }

  document.getElementById("explanation-text").textContent = res.explanation;
  document.getElementById("fuel-consumption-rate").textContent = `${res.currentL100km.toFixed(1)} L/100km`;
  document.getElementById("tractive-drag-val").textContent = `${Math.round(res.drag.total)} N`;

  drawGauge("speedGauge", state.speed, 0, 180, "km/h");
  drawGauge("rpmGauge", res.currentRpm, 0, state.vehicle.rpmRedline + 1000, "RPM", state.vehicle.rpmEcoMin, state.vehicle.rpmEcoMax, state.vehicle.rpmRedline);

  // Render Gear Matrix
  const tbody = document.getElementById("gear-matrix-body");
  tbody.innerHTML = "";
  res.gearEvaluations.forEach((item) => {
    const tr = document.createElement("tr");
    if (item.gear === state.engagedGear) tr.classList.add("active-gear");

    let tagClass = "optimal";
    if (item.status === "lugging") tagClass = "lugging";
    if (item.status === "overrev") tagClass = "overrev";

    tr.innerHTML = `
      <td>Gear ${item.gear} ${item.gear === state.engagedGear ? '(Current)' : ''}</td>
      <td>${state.vehicle.gearRatios[item.gear - 1].toFixed(2)}</td>
      <td>${item.rpm}</td>
      <td>${Math.round(res.drag.total * 0.15)}</td>
      <td><span class="status-tag ${tagClass}">${item.status}</span></td>
      <td>${item.suitability}</td>
    `;
    tbody.appendChild(tr);
  });

  // Fuel Analysis Tab
  document.getElementById("fuel-stat-current").textContent = `${res.currentL100km.toFixed(1)} L`;
  document.getElementById("fuel-stat-opt").textContent = `${res.optimalL100km.toFixed(1)} L`;
  const pctDiff = ((res.currentL100km - res.optimalL100km) / res.currentL100km) * 100;
  document.getElementById("fuel-savings-delta").textContent = 
    pctDiff > 0.5 ? `-${pctDiff.toFixed(1)}% potential consumption drop` : "Operating at peak efficiency";

  const totalD = res.drag.total || 1;
  document.getElementById("drag-aero-label").textContent = `${Math.round(res.drag.aero)} N`;
  document.getElementById("drag-rolling-label").textContent = `${Math.round(res.drag.rolling)} N`;
  document.getElementById("drag-grade-label").textContent = `${Math.round(res.drag.grade)} N`;
  document.getElementById("bar-aero").style.width = `${Math.min(100, (res.drag.aero / totalD) * 100)}%`;
  document.getElementById("bar-rolling").style.width = `${Math.min(100, (res.drag.rolling / totalD) * 100)}%`;
  document.getElementById("bar-grade").style.width = `${Math.min(100, Math.max(0, (res.drag.grade / totalD) * 100))}%`;

  renderEcoTips(res);
}

// 4. Dial Gauge Painter (Canvas)
function drawGauge(canvasId, value, minVal, maxVal, unit, ecoMin, ecoMax, redline) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  const w = canvas.width, h = canvas.height;
  const cx = w / 2, cy = h / 2, radius = w * 0.40;

  ctx.clearRect(0, 0, w, h);
  const startAngle = 0.75 * Math.PI, endAngle = 2.25 * Math.PI, sweepAngle = endAngle - startAngle;

  ctx.beginPath();
  ctx.arc(cx, cy, radius, startAngle, endAngle);
  ctx.lineWidth = 14;
  ctx.strokeStyle = "rgba(255, 255, 255, 0.06)";
  ctx.lineCap = "round";
  ctx.stroke();

  const valToAngle = (v) => {
    const fraction = (Math.max(minVal, Math.min(maxVal, v)) - minVal) / (maxVal - minVal);
    return startAngle + fraction * sweepAngle;
  };

  if (ecoMin !== undefined && ecoMax !== undefined) {
    ctx.beginPath();
    ctx.arc(cx, cy, radius, valToAngle(ecoMin), valToAngle(ecoMax));
    ctx.lineWidth = 14;
    ctx.strokeStyle = "rgba(0, 229, 153, 0.35)";
    ctx.stroke();
  }

  if (redline !== undefined) {
    ctx.beginPath();
    ctx.arc(cx, cy, radius, valToAngle(redline), endAngle);
    ctx.lineWidth = 14;
    ctx.strokeStyle = "rgba(255, 51, 102, 0.45)";
    ctx.stroke();
  }

  ctx.beginPath();
  ctx.arc(cx, cy, radius, startAngle, valToAngle(value));
  ctx.lineWidth = 14;
  ctx.strokeStyle = value >= redline ? "#ff3366" : "#00d2ff";
  ctx.lineCap = "round";
  ctx.stroke();
}

function renderEcoTips(res) {
  const container = document.getElementById("eco-tips-container");
  if (!container) return;
  container.innerHTML = "";

  const tips = [];
  if (state.speed > 105) {
    tips.push({
      title: "Aerodynamic Drag Threshold Exceeded",
      text: "Air drag increases quadratically with speed. Cruising at 95 km/h rather than 120 km/h drops fuel consumption by up to 18%."
    });
  }
  if (state.cargoKg > 150) {
    tips.push({
      title: "Excess Dead Weight",
      text: "Every 50 kg of excess weight increases rolling friction by ~2%. Remove unneeded payload to relieve low-gear strain."
    });
  }
  if (res.currentRpm > state.vehicle.rpmEcoMax) {
    tips.push({
      title: "Excess Pumping Work",
      text: "Upshifting earlier allows the engine to breathe with lower throttle restriction, maximizing thermal efficiency."
    });
  }
  tips.push({
    title: "Smooth Throttle Modulation",
    text: "Maintain a steady pedal position rather than cycling acceleration bursts to minimize transient enrichment fuel losses."
  });

  tips.forEach(t => {
    const card = document.createElement("div");
    card.style.background = "var(--bg-inset)";
    card.style.border = "1px solid var(--border-subtle)";
    card.style.borderRadius = "10px";
    card.style.padding = "1.1rem";
    card.innerHTML = `
      <div style="font-family: var(--font-display); font-size: 0.9rem; font-weight: 600; color: #fff; margin-bottom: 0.4rem;">${t.title}</div>
      <div style="font-size: 0.82rem; color: var(--text-muted); line-height: 1.45;">${t.text}</div>
    `;
    container.appendChild(card);
  });
}

// 5. DOM Initialization
document.addEventListener("DOMContentLoaded", () => {
  document.querySelectorAll(".tab-btn").forEach(btn => {
    btn.addEventListener("click", () => {
      document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
      document.querySelectorAll(".tab-content").forEach(c => c.style.display = "none");
      btn.classList.add("active");
      const target = document.getElementById(`tab-${btn.getAttribute("data-tab")}`);
      if (target) target.style.display = "block";
    });
  });

  const speedSlider = document.getElementById("input-speed");
  const gearSlider = document.getElementById("input-gear");

  speedSlider.addEventListener("input", (e) => {
    state.speed = parseInt(e.target.value, 10);
    refreshDashboard();
  });

  gearSlider.addEventListener("input", (e) => {
    state.engagedGear = parseInt(e.target.value, 10);
    refreshDashboard();
  });

  document.getElementById("input-gradient").addEventListener("input", (e) => {
    state.gradientPct = parseFloat(e.target.value);
    document.getElementById("val-gradient").textContent = `${state.gradientPct.toFixed(1)} %`;
    refreshDashboard();
  });

  document.getElementById("input-passengers").addEventListener("input", (e) => {
    state.passengers = parseInt(e.target.value, 10);
    document.getElementById("val-passengers").textContent = state.passengers;
    refreshDashboard();
  });

  document.getElementById("input-cargo").addEventListener("input", (e) => {
    state.cargoKg = parseInt(e.target.value, 10);
    document.getElementById("val-cargo").textContent = `${state.cargoKg} kg`;
    refreshDashboard();
  });

  document.getElementById("select-road-type").addEventListener("change", (e) => {
    state.roadType = e.target.value;
    refreshDashboard();
  });

  document.getElementById("btn-reset-conditions").addEventListener("click", () => {
    document.getElementById("input-gradient").value = 0;
    document.getElementById("input-passengers").value = 1;
    document.getElementById("input-cargo").value = 20;
    state.gradientPct = 0;
    state.passengers = 1;
    state.cargoKg = 20;
    document.getElementById("val-gradient").textContent = "0.0 %";
    document.getElementById("val-passengers").textContent = "1";
    document.getElementById("val-cargo").textContent = "20 kg";
    refreshDashboard();
  });

  document.getElementById("select-preset-vehicle").addEventListener("change", (e) => {
    state.activeVehicleKey = e.target.value;
    state.vehicle = JSON.parse(JSON.stringify(VEHICLE_PRESETS[state.activeVehicleKey]));
    
    document.getElementById("cfg-curb-weight").value = state.vehicle.curbWeight;
    document.getElementById("cfg-rpm-lug").value = state.vehicle.rpmLug;
    document.getElementById("cfg-rpm-eco-min").value = state.vehicle.rpmEcoMin;
    document.getElementById("cfg-rpm-eco-max").value = state.vehicle.rpmEcoMax;
    document.getElementById("cfg-rpm-redline").value = state.vehicle.rpmRedline;

    const gearMax = state.vehicle.gearRatios.length;
    gearSlider.max = gearMax;
    if (state.engagedGear > gearMax) state.engagedGear = gearMax;
    gearSlider.value = state.engagedGear;
    refreshDashboard();
  });

  document.getElementById("btn-apply-vehicle-cfg").addEventListener("click", () => {
    state.vehicle.curbWeight = parseFloat(document.getElementById("cfg-curb-weight").value) || state.vehicle.curbWeight;
    state.vehicle.rpmLug = parseInt(document.getElementById("cfg-rpm-lug").value, 10) || state.vehicle.rpmLug;
    state.vehicle.rpmEcoMin = parseInt(document.getElementById("cfg-rpm-eco-min").value, 10) || state.vehicle.rpmEcoMin;
    state.vehicle.rpmEcoMax = parseInt(document.getElementById("cfg-rpm-eco-max").value, 10) || state.vehicle.rpmEcoMax;
    state.vehicle.rpmRedline = parseInt(document.getElementById("cfg-rpm-redline").value, 10) || state.vehicle.rpmRedline;
    refreshDashboard();
    alert("Vehicle powertrain profile applied!");
  });

  document.getElementById("btn-save-scenario").addEventListener("click", () => {
    fetchShiftRecommendation().then(res => {
      const entry = {
        timestamp: new Date().toLocaleTimeString(),
        vehicle: state.vehicle.name,
        speed: state.speed,
        engagedGear: state.engagedGear,
        recGear: res.optimalGear,
        rpm: res.currentRpm,
        fuelRate: res.currentL100km.toFixed(1),
        advice: res.adviceType.toUpperCase()
      };
      state.scenarioHistory.unshift(entry);

      const hBody = document.getElementById("history-table-body");
      hBody.innerHTML = "";
      state.scenarioHistory.forEach(item => {
        const tr = document.createElement("tr");
        tr.innerHTML = `
          <td>${item.timestamp}</td>
          <td>${item.vehicle}</td>
          <td>${item.speed} km/h</td>
          <td>Gear ${item.engagedGear} ➔ ${item.recGear}</td>
          <td>${item.rpm}</td>
          <td>${item.fuelRate} L/100km</td>
          <td><span class="status-tag ${item.advice === 'OPTIMAL' ? 'optimal' : 'overrev'}">${item.advice}</span></td>
        `;
        hBody.appendChild(tr);
      });
      alert("Driving scenario saved to history table!");
    });
  });

  document.getElementById("btn-export-csv").addEventListener("click", () => {
    if (state.scenarioHistory.length === 0) {
      alert("No logged scenarios available to export.");
      return;
    }
    let csv = "Timestamp,Vehicle,Speed (km/h),Engaged Gear,Recommended Gear,RPM,Est Fuel (L/100km),Advice\n";
    state.scenarioHistory.forEach(r => {
      csv += `"${r.timestamp}","${r.vehicle}",${r.speed},${r.engagedGear},${r.recGear},${r.rpm},${r.fuelRate},"${r.advice}"\n`;
    });
    const blob = new Blob([csv], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `gear_optimization_report_${Date.now()}.csv`;
    a.click();
  });

  refreshDashboard();
});