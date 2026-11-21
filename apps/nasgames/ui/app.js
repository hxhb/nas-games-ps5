(() => {
  "use strict";

  const $ = (id) => document.getElementById(id);
  const ui = {
    shell: $("shell"),
    launchScreen: $("launchScreen"),
    launchScreenTitle: $("launchScreenTitle"),
    appVersion: $("appVersion"),
    filters: $("filters"),
    formatFilter: $("formatFilter"),
    formatFilterButton: $("formatFilterButton"),
    formatFilterValue: $("formatFilterValue"),
    formatMenu: $("formatMenu"),
    search: $("search"),
    scanButton: $("scanButton"),
    settingsButton: $("settingsButton"),
    gameCount: $("gameCount"),
    sourceSummary: $("sourceSummary"),
    libraryStats: $("libraryStats"),
    noticeStrip: $("noticeStrip"),
    library: $("library"),
    scanStatus: $("scanStatus"),
    sourcePills: $("sourcePills"),
    detailPanel: $("detailPanel"),
    detailBackdrop: $("detailBackdrop"),
    detailClose: $("detailClose"),
    detailArt: $("detailArt"),
    detailCover: $("detailCover"),
    detailFormat: $("detailFormat"),
    detailTitle: $("detailTitle"),
    detailSubtitle: $("detailSubtitle"),
    detailPath: $("detailPath"),
    detailStorage: $("detailStorage"),
    launchButton: $("launchButton"),
    launchButtonText: $("launchButtonText"),
    installButton: $("installButton"),
    detailActionTitle: $("detailActionTitle"),
    detailInstallStatus: $("detailInstallStatus"),
    detailInstalledState: $("detailInstalledState"),
    detailInstalledMeta: $("detailInstalledMeta"),
    detailHint: $("detailHint"),
    detailPackages: $("detailPackages"),
    detailPackagesCount: $("detailPackagesCount"),
    detailPackageList: $("detailPackageList"),
    detailTechNote: $("detailTechNote"),
    transferShell: $("transferShell"),
    transferButton: $("transferButton"),
    transferPanel: $("transferPanel"),
    transferClose: $("transferClose"),
    transferBadge: $("transferBadge"),
    transferTitle: $("transferTitle"),
    transferStateDot: $("transferStateDot"),
    transferStage: $("transferStage"),
    transferPercent: $("transferPercent"),
    transferMessage: $("transferMessage"),
    transferSize: $("transferSize"),
    transferSpeed: $("transferSpeed"),
    transferDone: $("transferDone"),
    transferQueueCount: $("transferQueueCount"),
    transferQueueList: $("transferQueueList"),
    transferHistoryCount: $("transferHistoryCount"),
    transferHistoryList: $("transferHistoryList"),
    clearHistoryButton: $("clearHistoryButton"),
    jobProgress: $("jobProgress"),
    cancelJobButton: $("cancelJobButton"),
    settingsPanel: $("settingsPanel"),
    settingsClose: $("settingsClose"),
    settingsNav: $("settingsNav"),
    settingsNavConnections: $("settingsNavConnections"),
    connectionsToolbarCount: $("connectionsToolbarCount"),
    addSmbButton: $("addSmbButton"),
    addFtpButton: $("addFtpButton"),
    addLocalButton: $("addLocalButton"),
    connectionsList: $("connectionsList"),
    settingsConnectionCount: $("settingsConnectionCount"),
    settingsConnectionDetail: $("settingsConnectionDetail"),
    settingsGameCount: $("settingsGameCount"),
    settingsGameDetail: $("settingsGameDetail"),
    settingsAttentionCount: $("settingsAttentionCount"),
    settingsAttentionDetail: $("settingsAttentionDetail"),
    settingsAttentionList: $("settingsAttentionList"),
    settingsHealth: $("settingsHealth"),
    settingPort: $("settingPort"),
    settingInterval: $("settingInterval"),
    settingDepth: $("settingDepth"),
    settingAutoScan: $("settingAutoScan"),
    settingOpenBrowser: $("settingOpenBrowser"),
    settingKeepAwake: $("settingKeepAwake"),
    settingsHint: $("settingsHint"),
    settingsCancel: $("settingsCancel"),
    settingsSave: $("settingsSave"),
    editorPanel: $("editorPanel"),
    editorTitle: $("editorTitle"),
    editorClose: $("editorClose"),
    editorName: $("editorName"),
    editorType: $("editorType"),
    editorEnabled: $("editorEnabled"),
    editorStepServer: $("editorStepServer"),
    editorStepShare: $("editorStepShare"),
    editorStepRoots: $("editorStepRoots"),
    editorProfile: $("editorProfile"),
    editorProfileIcon: $("editorProfileIcon"),
    editorProfileTitle: $("editorProfileTitle"),
    editorProfileHint: $("editorProfileHint"),
    editorProfileState: $("editorProfileState"),
    networkFields: $("networkFields"),
    editorHost: $("editorHost"),
    editorPort: $("editorPort"),
    editorShare: $("editorShare"),
    shareField: $("shareField"),
    enumerateSharesButton: $("enumerateSharesButton"),
    editorUser: $("editorUser"),
    editorPassword: $("editorPassword"),
    passwordHint: $("passwordHint"),
    domainField: $("domainField"),
    editorDomain: $("editorDomain"),
    localRootField: $("localRootField"),
    editorLocalRoot: $("editorLocalRoot"),
    browseRootButton: $("browseRootButton"),
    rootsList: $("rootsList"),
    editorRootsCount: $("editorRootsCount"),
    smbNote: $("smbNote"),
    smbPreview: $("smbPreview"),
    smbGuideStatus: $("smbGuideStatus"),
    smbGuideHost: $("smbGuideHost"),
    smbGuideAuth: $("smbGuideAuth"),
    smbGuideShare: $("smbGuideShare"),
    smbGuideHint: $("smbGuideHint"),
    sharePicker: $("sharePicker"),
    shareResults: $("shareResults"),
    closeSharePicker: $("closeSharePicker"),
    editorError: $("editorError"),
    editorTestFlow: $("editorTestFlow"),
    editorTestSummary: $("editorTestSummary"),
    editorTestSteps: $("editorTestSteps"),
    editorTestHint: $("editorTestHint"),
    editorTest: $("editorTest"),
    editorCancel: $("editorCancel"),
    editorApply: $("editorApply"),
    browserPanel: $("browserPanel"),
    browserClose: $("browserClose"),
    browserUp: $("browserUp"),
    browserPath: $("browserPath"),
    browserGo: $("browserGo"),
    browserRefresh: $("browserRefresh"),
    browserSourceLabel: $("browserSourceLabel"),
    browserBreadcrumbs: $("browserBreadcrumbs"),
    browserRootStatus: $("browserRootStatus"),
    browserList: $("browserList"),
    browserHint: $("browserHint"),
    browserCancel: $("browserCancel"),
    browserChoose: $("browserChoose"),
    logText: $("logText"),
    logRefresh: $("logRefresh"),
    confirmPanel: $("confirmPanel"),
    confirmTitle: $("confirmTitle"),
    confirmMessage: $("confirmMessage"),
    confirmCancel: $("confirmCancel"),
    confirmOk: $("confirmOk"),
    toastStack: $("toastStack"),
  };

  const app = {
    state: null,
    settings: null,
    filter: "all",
    format: "all",
    query: "",
    selectedGameId: null,
    librarySignature: "",
    transferDismissedFor: "",
    lastJobKey: "",
    transferAutoHideTimer: 0,
    transferFollowupTimer: 0,
    cancelBusy: false,
    launchBusy: false,
    launchScreenActive: false,
    launchScreenGameId: "",
    launchResetTimer: 0,
    statusTimer: 0,
    logTimer: 0,
    logBusy: false,
    refreshBusy: false,
    refreshFailed: false,
    loadToken: 0,
    editor: null,
    shares: [],
    browser: { source: null, path: "/", token: 0 },
    confirmCallback: null,
    confirmBusy: false,
    scanPending: false,
    scanSequence: null,
    settingsTab: "overview",
    groups: [],
    groupsRevision: -1,
    detailKey: "",
    headerKey: "",
    transferQueueKey: "",
    transferHistoryKey: "",
    settingsOverviewKey: "",
    sizeStates: Object.create(null),
    preloadedArt: new Set(),
    focusCache: null,
    focusReturn: Object.create(null),
  };

  const FORMAT_LABELS = {
    all: "全部",
    folder: "文件夹",
    pkg: "PKG",
  };

  function element(tag, className, text) {
    const result = document.createElement(tag);
    if (className) result.className = className;
    if (text !== undefined && text !== null) result.textContent = String(text);
    return result;
  }

  function clone(value) {
    return JSON.parse(JSON.stringify(value));
  }

  function setText(node, text) {
    const value = text === undefined || text === null ? "" : String(text);
    if (node.textContent !== value) node.textContent = value;
  }

  async function api(path, options = {}) {
    const request = {
      method: options.method || "GET",
      cache: "no-store",
      headers: options.headers ? Object.assign({}, options.headers) : {},
    };
    if (request.method !== "GET" && request.method !== "HEAD") {
      request.headers["Content-Type"] = "application/json";
    }
    if (options.body !== undefined) {
      request.body = typeof options.body === "string"
        ? options.body
        : JSON.stringify(options.body);
    }
    const response = await fetch(path, request);
    const text = await response.text();
    let data = {};
    if (text) {
      try {
        data = JSON.parse(text);
      } catch (error) {
        data = { ok: false, error: text };
      }
    }
    if (!response.ok || data.ok === false) {
      throw new Error((data && data.error) || `HTTP ${response.status}`);
    }
    return data;
  }

  function showToast(message, type = "") {
    const toast = element("div", `toast ${type}`.trim(), message);
    ui.toastStack.appendChild(toast);
    while (ui.toastStack.children.length > 4) {
      ui.toastStack.removeChild(ui.toastStack.firstElementChild);
    }
    window.setTimeout(() => {
      toast.style.opacity = "0";
      toast.style.transform = "translateX(12px)";
      window.setTimeout(() => toast.remove(), 220);
    }, 4300);
  }

  function formatBytes(value) {
    const bytes = Number(value) || 0;
    if (!bytes) return "0 B";
    const units = ["B", "KB", "MB", "GB", "TB"];
    const index = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
    const number = bytes / Math.pow(1024, index);
    return `${number >= 100 || index === 0 ? number.toFixed(0) : number.toFixed(1)} ${units[index]}`;
  }

  function pluralizeGames(count) {
    return `${count} 个游戏`;
  }

  function packageKindLabel(game) {
    if (game.pkgKind === "update") return "补丁";
    if (game.pkgKind === "dlc") return "DLC";
    if (game.pkgKind === "base") return "基础包";
    return game.type === "pkg" ? "PKG" : "文件夹";
  }

  function installStatusInfo(game) {
    if (game.installStatus === "superseded") {
      return {
        label: game.installedVersion ? `已由 ${game.installedVersion} 覆盖` : "已由更高版本覆盖",
        tone: "good",
        disabled: true,
      };
    }
    if (game.installed || game.installStatus === "installed") {
      return { label: "已安装", tone: "good", disabled: true };
    }
    if (game.installStatus === "incomplete" || game.installPartial) {
      return {
        label: game.baseInstalled === false ? "载荷不完整 · 需基础包" : "载荷不完整，可重新安装",
        tone: "warn",
        disabled: game.baseInstalled === false,
      };
    }
    if (game.installStatus === "unknown" || game.installKnown === false) {
      return { label: "版本/标识未知", tone: "warn", disabled: false };
    }
    if ((game.pkgKind === "update" || game.pkgKind === "dlc") &&
        (game.installStatus === "needs-base" || !game.baseInstalled)) {
      return { label: "需先安装基础包", tone: "warn", disabled: true };
    }
    return { label: "可安装", tone: "", disabled: false };
  }

  function gameSortTitle(game) {
    return (game.title || game.titleId || game.path || "").toLocaleLowerCase();
  }

  function compareGames(a, b) {
    return gameSortTitle(a).localeCompare(gameSortTitle(b), "zh-CN", {
      numeric: true,
      sensitivity: "base",
    });
  }

  function createLibraryStat(label, value, tone) {
    const stat = element("span", `library-stat ${tone || ""}`.trim());
    stat.appendChild(element("strong", "", value));
    stat.appendChild(document.createTextNode(label));
    return stat;
  }

  function renderLibraryStats(groups) {
    ui.libraryStats.replaceChildren();
    if (!groups.length) return;
    const installed = groups.filter((group) => group.installed).length;
    ui.libraryStats.appendChild(createLibraryStat("已安装", installed, "good"));
    ui.libraryStats.appendChild(createLibraryStat(
      "未安装", groups.length - installed, installed ? "" : "accent"));
  }

  function renderFilterCounts(groups) {
    const counts = {
      all: groups.length,
      folder: groups.filter((group) => group.hasFolder).length,
      pkg: groups.filter((group) => group.hasPkg).length,
      installed: groups.filter((group) => group.installed).length,
      notInstalled: groups.filter((group) => !group.installed).length,
    };
    for (const button of ui.filters.querySelectorAll("[data-filter]")) {
      let badge = button.querySelector(".filter-count");
      if (!badge) {
        badge = element("span", "filter-count");
        button.appendChild(badge);
      }
      setText(badge, counts[button.dataset.filter] || 0);
    }
    for (const badge of ui.formatMenu.querySelectorAll("[data-format-count]")) {
      setText(badge, counts[badge.dataset.formatCount] || 0);
    }
    updateFormatFilterValue();
  }

  function updateFormatFilterValue() {
    const label = FORMAT_LABELS[app.format] || FORMAT_LABELS.all;
    setText(ui.formatFilterValue, label);
    ui.formatFilterButton.setAttribute("aria-label",
      `格式筛选，当前${label}`);
    ui.formatFilterButton.title = "打开格式筛选菜单";
    for (const option of ui.formatMenu.querySelectorAll("[data-format]")) {
      const active = option.dataset.format === app.format;
      option.classList.toggle("is-active", active);
      option.setAttribute("aria-checked", active ? "true" : "false");
    }
  }

  function setFormat(format, options = {}) {
    app.format = Object.prototype.hasOwnProperty.call(FORMAT_LABELS, format)
      ? format : "all";
    updateFormatFilterValue();
    setFormatMenu(false);
    if (options.focusButton) {
      window.requestAnimationFrame(() => {
        ui.formatFilterButton.focus({ preventScroll: true });
      });
    }
    renderLibrary(true);
  }

  function setFormatMenu(open) {
    if (ui.formatMenu.hidden === !open) return;
    ui.formatMenu.hidden = !open;
    ui.formatFilterButton.classList.toggle("is-open", open);
    ui.formatFilterButton.setAttribute("aria-expanded", open ? "true" : "false");
    invalidateFocusCache();
    if (!open) return;
    const selected = ui.formatMenu.querySelector(".format-option.is-active") ||
      ui.formatMenu.querySelector(".format-option");
    window.requestAnimationFrame(() => {
      if (selected && !ui.formatMenu.hidden) {
        selected.focus({ preventScroll: true });
      }
    });
  }

  function sourceTypeLabel(type) {
    if (type === "smb") return "SMB";
    if (type === "ftp") return "FTP";
    if (type === "local") return "LOCAL";
    return String(type || "NONE").toUpperCase();
  }

  function sourceEndpoint(source) {
    if (source.type === "local") return source.localRoot || "未设置目录";
    const port = source.port ? `:${source.port}` : "";
    if (source.type === "smb") {
      return `${source.host || "?"}${port}/${source.share || "?"}`;
    }
    return `${source.host || "?"}${port}`;
  }

  function normalizePath(value) {
    let path = String(value || "/").trim().replace(/\\/g, "/");
    if (!path.startsWith("/")) path = `/${path}`;
    const parts = [];
    for (const part of path.split("/")) {
      if (!part || part === ".") continue;
      if (part === "..") {
        parts.pop();
      } else {
        parts.push(part);
      }
    }
    return `/${parts.join("/")}`;
  }

  function parentPath(path) {
    const normalized = normalizePath(path);
    if (normalized === "/") return "/";
    const index = normalized.lastIndexOf("/");
    return index <= 0 ? "/" : normalized.slice(0, index);
  }

  function sourceRequestPayload(source) {
    return clone(source);
  }

  function formatFileTime(value) {
    const seconds = Number(value);
    if (!Number.isFinite(seconds) || seconds <= 0) return "—";
    const date = new Date(seconds * 1000);
    const pad = (number) => String(number).padStart(2, "0");
    return `${date.getFullYear()}/${pad(date.getMonth() + 1)}/${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}`;
  }

  function parseShareAddress(rawShare, rawHost) {
    let host = String(rawHost || "").trim();
    let value = String(rawShare || "").trim().replace(/\\/g, "/");
    const qualified = /^smb:\/\//i.test(value) || value.startsWith("//");
    if (/^smb:\/\//i.test(value)) value = value.replace(/^smb:\/\//i, "");
    const segments = value.split("/").filter(Boolean);
    let share = "";
    if (qualified) host = segments.shift() || host;
    share = segments.shift() || "";
    const subdirectories = segments.filter((part) => part && part !== ".");
    return {
      host,
      share,
      suffix: subdirectories.length ? normalizePath(`/${subdirectories.join("/")}`) : "/",
    };
  }

  function findGame(id) {
    if (!app.state || !Array.isArray(app.state.games)) return null;
    return app.state.games.find((game) => game.id === id) || null;
  }

  function packageRank(game) {
    if (game.type === "folder" && game.packing === "plain") return 0;
    if (game.pkgKind === "base") return 1;
    if (game.type === "folder") return 2;
    if (game.pkgKind === "unknown") return 5;
    return 10;
  }

  function comparePackages(a, b) {
    const rank = packageRank(a) - packageRank(b);
    if (rank) return rank;
    return String(b.version || "").localeCompare(String(a.version || ""), "zh-CN", {
      numeric: true,
      sensitivity: "base",
    });
  }

  function compareContentPackages(a, b) {
    const rank = (a.pkgKind === "update" ? 0 : 1) - (b.pkgKind === "update" ? 0 : 1);
    if (rank) return rank;
    if (!!a.installed !== !!b.installed) return a.installed ? 1 : -1;
    return String(b.version || "").localeCompare(String(a.version || ""), "zh-CN", {
      numeric: true,
      sensitivity: "base",
    });
  }

  function buildGameGroups(games) {
    const map = new Map();
    for (const game of games) {
      const key = game.titleId ? `tid:${game.titleId}` : `game:${game.id}`;
      let group = map.get(key);
      if (!group) {
        group = { key, games: [] };
        map.set(key, group);
      }
      group.games.push(game);
    }

    const groups = [];
    for (const group of map.values()) {
      group.games.sort(comparePackages);
      const primary = group.games[0];
      const iconGame = group.games.find((game) => game.icon) || primary;
      const backdropGame = group.games.find((game) => game.pic0 || game.pic1) || primary;
      const contentPackages = group.games
        .filter((game) => game.pkgKind === "update" || game.pkgKind === "dlc")
        .sort(compareContentPackages);
      const baseGames = group.games.filter((game) => game.pkgKind === "base");
      const baseInstalled = baseGames.some((game) => game.installed) ||
        group.games.some((game) => game.baseInstalled);
      groups.push({
        id: primary.id,
        key: group.key,
        primary,
        games: group.games,
        packages: contentPackages,
        title: primary.title || primary.titleId || "未命名游戏",
        titleId: primary.titleId,
        version: primary.version,
        icon: iconGame.icon,
        pic0: backdropGame.pic0,
        pic1: backdropGame.pic1,
        sourceName: primary.sourceName,
        path: primary.path,
        type: primary.type,
        pkgKind: primary.pkgKind,
        packing: primary.packing,
        size: primary.size,
        installed: baseInstalled,
        baseInstalled,
        launchable: group.games.some((game) => !!game.launchable),
        primaryIsContent: primary.pkgKind === "update" || primary.pkgKind === "dlc",
        hasBaseResource: group.games.some((game) =>
          game.type === "folder" || game.pkgKind === "base"),
        hasPkg: group.games.some((game) => game.type === "pkg"),
        hasFolder: group.games.some((game) => game.type === "folder"),
        patchCount: contentPackages.filter((game) => game.pkgKind === "update").length,
        dlcCount: contentPackages.filter((game) => game.pkgKind === "dlc").length,
        pendingPatches: contentPackages.filter((game) =>
          game.pkgKind === "update" && !game.installed).length,
        pendingDlc: contentPackages.filter((game) =>
          game.pkgKind === "dlc" && !game.installed).length,
        incompleteContent: contentPackages.filter((game) =>
          game.installPartial || game.installStatus === "incomplete").length,
      });
    }
    return groups;
  }

  function gameGroups() {
    if (!app.state || !Array.isArray(app.state.games)) return [];
    const revision = Number(app.state.catalogRevision) || 0;
    if (app.groupsRevision !== revision) {
      app.groups = buildGameGroups(app.state.games);
      app.groupsRevision = revision;
    }
    return app.groups;
  }

  function findGameGroup(id) {
    return gameGroups().find((group) =>
      group.id === id || group.games.some((game) => game.id === id)) || null;
  }

  function gameSizeInfo(group, game) {
    const measured = app.sizeStates[game.id];
    const size = Number(group.size) || Number(game.size) ||
      Number(measured && measured.size) || 0;
    if (size > 0 || (measured && measured.status === "known")) return { size, status: "known" };
    if (measured && measured.status === "error") return { size: 0, status: "error" };
    return { size: 0, status: "loading" };
  }

  function detailFormatInfo(game, group) {
    if (game.type === "pkg") {
      const name = game.pkgKind === "update"
        ? "PKG 补丁包"
        : game.pkgKind === "dlc" ? "PKG DLC 内容包" : "PKG 安装包";
      return {
        kind: "package",
        name,
        token: "系统安装器",
        detail: [group.hasFolder ? "另有文件夹资源" : ""]
          .filter(Boolean).join(" · "),
        tone: "format-pkg",
      };
    }
    const packing = game.packing === "zstd" ? "ZSTD" :
      game.packing === "lz4" ? "LZ4" : "未压缩";
    return {
      kind: "folder",
      name: "文件夹游戏",
      token: packing,
      detail: [game.packing !== "plain" ? "压缩文件夹" : "原始文件夹",
        group.hasPkg ? "另有 PKG 资源" : ""]
        .filter(Boolean).join(" · "),
      tone: "format-folder",
    };
  }

  function detailFormatIcon(kind) {
    const paths = kind === "package"
      ? '<path d="M11 21.73a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73z"></path>' +
        '<path d="M12 22V12"></path>' +
        '<path d="m3.3 7 7.7 4.4a2 2 0 0 0 2 0L20.7 7"></path>' +
        '<path d="m7.5 4.27 9 5.15"></path>'
      : '<path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"></path>';
    return `<svg viewBox="0 0 24 24" aria-hidden="true">${paths}</svg>`;
  }

  function requestGameSize(group, game) {
    if (game.type !== "folder" || gameSizeInfo(group, game).size > 0) return;
    const previous = app.sizeStates[game.id];
    if (previous) return;
    const pending = { status: "loading", size: 0 };
    app.sizeStates[game.id] = pending;
    const refreshSize = () => {
      const selected = findGameGroup(app.selectedGameId);
      if (!ui.detailPanel.hidden && selected && selected.primary.id === game.id) {
        renderDetail(true);
      }
    };
    api(`/api/game-size?id=${encodeURIComponent(game.id)}`)
      .then((data) => {
        if (app.sizeStates[game.id] !== pending) return;
        const size = Number(data.size) || 0;
        app.sizeStates[game.id] = {
          status: "known",
          size,
          files: Number(data.files) || 0,
        };
        refreshSize();
      })
      .catch(() => {
        if (app.sizeStates[game.id] !== pending) return;
        app.sizeStates[game.id] = { status: "error", size: 0 };
        refreshSize();
      });
  }

  function preloadDetailArt(group) {
    if (!group) return;
    const url = group.pic0 || group.pic1 || group.icon;
    if (!url || app.preloadedArt.has(url)) return;
    app.preloadedArt.add(url);
    const image = new Image();
    image.onerror = () => app.preloadedArt.delete(url);
    image.decoding = "async";
    image.src = url;
  }

  function installTargetInfo(target, game) {
    if (target === "system") {
      return { key: "system", label: "PS5 系统盘", path: "/user" };
    }
    if (target === "homebrew" ||
        (game.type === "folder" && game.packing !== "plain")) {
      return { key: "homebrew", label: "PS5 本机应用目录", path: "/data/homebrew" };
    }
    return { key: "library", label: "Game Center 本地库", path: "/data/nasgames/library" };
  }

  function installPreflight(game, target) {
    const targetInfo = installTargetInfo(target, game);
    const storage = app.state && app.state.storage
      ? app.state.storage[targetInfo.key] : null;
    const group = findGameGroup(game.id);
    const sizeInfo = group && group.primary === game
      ? gameSizeInfo(group, game)
      : { size: Number(game.size) || 0, status: Number(game.size) ? "known" : "loading" };
    const required = Number(game.size) || Number(sizeInfo.size) || 0;
    const registrationOnly = target === "library" && !!game.libraryCopy;
    const free = storage && storage.known ? Number(storage.free) || 0 : 0;
    const freeKnown = !!(storage && storage.known);
    return {
      targetInfo,
      required,
      registrationOnly,
      sizeStatus: sizeInfo.status,
      free,
      freeKnown,
      insufficient: !registrationOnly && freeKnown && required > 0 && required > free,
      remaining: freeKnown && required > 0 ? Math.max(0, free - required) : 0,
      shortfall: freeKnown && required > free ? required - free : 0,
    };
  }

  function findStateSource(id) {
    if (!app.state || !Array.isArray(app.state.sources)) return null;
    return app.state.sources.find((source) => source.id === id) || null;
  }

  function filteredGames() {
    if (!app.state || !Array.isArray(app.state.games)) return [];
    const query = app.query.trim().toLocaleLowerCase();
    return gameGroups().filter((group) => {
      if (app.format === "pkg" && !group.hasPkg) return false;
      if (app.format === "folder" && !group.hasFolder) return false;
      if (app.filter === "installed" && !group.installed) return false;
      if (app.filter === "notInstalled" && group.installed) return false;
      if (!query) return true;
      const haystack = group.games.map((game) => [
        game.title, game.titleId, game.sourceName, game.path, game.version,
        game.pkgKind, game.contentId,
      ].join(" ")).join(" ").toLocaleLowerCase();
      return haystack.includes(query);
    });
  }

  function createImage(url, alt) {
    if (!url) return null;
    const image = document.createElement("img");
    image.src = url;
    image.alt = alt || "";
    image.loading = "lazy";
    image.decoding = "async";
    image.draggable = false;
    return image;
  }

  function createCover(game, className) {
    const cover = element("div", className || "game-cover");
    const placeholder = element("div", "cover-placeholder");
    const monogram = (game.title || game.titleId || "?").trim().slice(0, 2).toUpperCase();
    placeholder.appendChild(element("span", "", monogram));
    cover.appendChild(placeholder);
    const image = createImage(game.icon || game.pic0, game.title);
    if (image) {
      image.addEventListener("load", () => { placeholder.hidden = true; });
      image.addEventListener("error", () => { image.remove(); placeholder.hidden = false; });
      cover.appendChild(image);
    }
    return cover;
  }

  function createGameCard(game, index) {
    const card = element("button", game.installed ? "game-card is-installed" : "game-card");
    card.type = "button";
    card.dataset.gameId = game.id;
    card.style.setProperty("--card-index", String(Math.min(Number(index) || 0, 9)));
    card.setAttribute("aria-label",
      `${game.title || game.titleId || "未命名游戏"}，${game.installed ? "基础游戏已安装" : "基础游戏未安装"}`);

    const cover = createCover(game, "game-cover");
    const bottom = element("div", "card-bottom");
    const formats = element("div", "card-format-row");
    formats.appendChild(element("span",
      `card-format-badge ${game.type === "pkg" ? "pkg" : "folder"}`,
      game.type === "pkg" ? "PKG" : "FOLDER"));
    if (game.packing && game.packing !== "plain") {
      formats.appendChild(element("span", "card-format-badge packing",
        game.packing.toUpperCase()));
    }
    if (game.patchCount || game.dlcCount) {
      if (game.patchCount) {
        formats.appendChild(element("span",
          `card-format-badge count ${game.pendingPatches ? "attention" : ""}`.trim(),
          `${game.patchCount} 补丁`));
      }
      if (game.dlcCount) {
        formats.appendChild(element("span",
          `card-format-badge count ${game.pendingDlc ? "attention" : ""}`.trim(),
          `${game.dlcCount} DLC`));
      }
    }
    bottom.appendChild(formats);
    const installState = element("span",
      `card-install-state ${game.installed ? "installed" : "available"}`,
      game.installed ? "已安装" : "未安装");
    if (game.pendingPatches || game.pendingDlc || game.incompleteContent) {
      installState.classList.add("has-pending");
      installState.title = [
        game.pendingPatches ? `${game.pendingPatches} 个补丁未安装` : "",
        game.pendingDlc ? `${game.pendingDlc} 个 DLC 未安装` : "",
        game.incompleteContent ? `${game.incompleteContent} 个内容载荷不完整` : "",
      ].filter(Boolean).join("，");
    }
    bottom.appendChild(installState);
    cover.appendChild(bottom);
    card.appendChild(cover);

    const copy = element("div", "card-copy");
    copy.appendChild(element("h3", "", game.title || game.titleId || "未命名游戏"));
    const detail = [game.titleId, game.version].filter(Boolean).join(" · ");
    copy.appendChild(element("p", "", detail || (game.installed ? "已在 PS5 上安装" : "可安装")));
    card.appendChild(copy);
    card.addEventListener("click", () => openDetail(game.id));
    return card;
  }

  function appendLibraryGroup(fragment, title, state, games, description) {
    if (!games.length) return;
    const group = element("section", `library-group ${state}`);
    const head = element("div", "library-group-head");
    const label = element("div", "library-group-label");
    label.appendChild(element("span", "library-group-dot"));
    const copy = element("span", "library-group-copy");
    copy.appendChild(element("strong", "", title));
    copy.appendChild(element("small", "", description));
    label.appendChild(copy);
    head.appendChild(label);
    head.appendChild(element("span", "library-group-count", `${games.length} 款`));
    group.appendChild(head);
    const grid = element("div", "library-group-grid");
    for (let index = 0; index < games.length; index++) {
      grid.appendChild(createGameCard(games[index], index));
    }
    group.appendChild(grid);
    fragment.appendChild(group);
  }

  function renderLibrary(force) {
    if (!app.state) return;
    const signature = [
      Number(app.state.catalogRevision) || 0,
      app.filter,
      app.format,
      app.query,
      (app.state.sources || []).map((source) =>
        [source.id, source.name, source.status, source.error || "",
          source.enabled ? 1 : 0].join(":")).join("|"),
    ].join("\n");
    if (!force && signature === app.librarySignature) return;
    app.librarySignature = signature;
    const games = filteredGames();

    const focused = document.activeElement;
    const focusedGameId = focused && focused.dataset ? focused.dataset.gameId : "";
    ui.library.replaceChildren();

    if (!games.length) {
      ui.library.appendChild(createEmptyState());
    } else {
      const fragment = document.createDocumentFragment();
      const installed = games.filter((game) => game.installed).sort(compareGames);
      const notInstalled = games.filter((game) => !game.installed).sort(compareGames);
      appendLibraryGroup(fragment, "已安装", "installed", installed,
        "已注册到 PS5");
      appendLibraryGroup(fragment, "未安装", "not-installed", notInstalled,
        "来自 NAS，可安装");
      ui.library.appendChild(fragment);
    }

    if (focusedGameId) {
      const next = ui.library.querySelector(`[data-game-id="${CSS.escape(focusedGameId)}"]`);
      if (next) next.focus({ preventScroll: true });
    }
    invalidateFocusCache();
  }

  function createEmptyState() {
    const container = element("div", "empty-state");
    container.appendChild(element("div", "empty-icon", app.state.sources.length ? "⌕" : "＋"));
    if (!app.state.sources.length) {
      container.appendChild(element("h2", "", "先连接你的 NAS"));
      const copy = element("p", "", "添加 SMB 或 FTP 连接，选择游戏目录后，Game Center 会扫描文件夹游戏与 PKG，并把封面显示在这里。");
      container.appendChild(copy);
      const button = element("button", "primary-button", "添加第一个连接");
      button.type = "button";
      button.addEventListener("click", openSettings);
      container.appendChild(button);
      return container;
    }
    if (!app.state.games.length) {
      container.appendChild(element("h2", "", "还没有发现游戏"));
      const failed = app.state.sources.filter((source) => source.status === "error");
      const message = failed.length
        ? `有 ${failed.length} 个连接读取失败。请检查地址、账号和扫描目录，然后重新扫描。`
        : "已连接来源，但扫描目录中没有找到含 eboot.bin + sce_sys 的文件夹游戏或 .pkg 文件。";
      container.appendChild(element("p", "", message));
      const row = element("div", "button-row");
      const scan = element("button", "primary-button", "重新扫描");
      scan.type = "button";
      scan.addEventListener("click", requestScan);
      const settings = element("button", "secondary-button", "检查连接");
      settings.type = "button";
      settings.addEventListener("click", openSettings);
      row.appendChild(scan);
      row.appendChild(settings);
      container.appendChild(row);
      return container;
    }
    container.appendChild(element("div", "empty-icon", "⌕"));
    container.appendChild(element("h2", "", "没有符合条件的游戏"));
    container.appendChild(element("p", "", "尝试切换筛选条件或清空搜索关键字。"));
    const button = element("button", "secondary-button", "清空筛选");
    button.type = "button";
    button.addEventListener("click", () => {
      app.filter = "all";
      app.format = "all";
      app.query = "";
      ui.search.value = "";
      for (const filter of ui.filters.querySelectorAll("[data-filter]")) {
        filter.classList.toggle("is-active", filter.dataset.filter === "all");
      }
      updateFormatFilterValue();
      setFormatMenu(false);
      renderLibrary(true);
    });
    container.appendChild(button);
    return container;
  }

  function renderHeaderState() {
    if (!app.state) return;
    setText(ui.appVersion, app.state.version);
    const sources = app.state.sources || [];
    const scan = app.state.scan || {};
    const headerKey = [
      Number(app.state.catalogRevision) || 0,
      scan.busy ? 1 : 0,
      scan.startedAt || 0,
      scan.finishedAt || 0,
      scan.message || "",
      app.scanPending ? 1 : 0,
      app.refreshFailed ? 1 : 0,
      sources.map((source) =>
        `${source.id}:${source.status}:${source.error || ""}:${source.enabled ? 1 : 0}`
      ).join("|"),
    ].join("\n");
    if (headerKey === app.headerKey) return;
    app.headerKey = headerKey;

    const groups = gameGroups();
    const online = sources.filter((source) => source.status === "online").length;
    const installed = groups.filter((game) => game.installed).length;
    setText(ui.gameCount, `${pluralizeGames(groups.length)} · 已安装 ${installed}`);
    renderLibraryStats(groups);
    renderFilterCounts(groups);
    setText(ui.sourceSummary, sources.length
      ? `${sources.length} 个连接 · ${online} 在线`
      : "未配置连接");

    ui.scanStatus.classList.toggle("busy", !!scan.busy || app.scanPending);
    ui.scanStatus.classList.toggle("error", sources.some((source) => source.status === "error"));
    const scanText = ui.scanStatus.querySelector("span:last-child");
    setText(scanText, scan.busy ? "正在扫描远端目录" : (scan.message || "等待扫描"));
    ui.scanButton.classList.toggle("spinning", !!scan.busy || app.scanPending);

    ui.sourcePills.replaceChildren();
    for (const source of sources) {
      const pill = element("span", `source-pill ${source.status || ""}`.trim());
      pill.appendChild(element("i"));
      pill.appendChild(document.createTextNode(source.name || source.id));
      if (source.error) pill.title = source.error;
      ui.sourcePills.appendChild(pill);
    }

    const failing = sources.find((source) => source.status === "error" && source.error);
    if (failing) {
      ui.noticeStrip.hidden = false;
      setText(ui.noticeStrip, `${failing.name || failing.id}: ${failing.error}`);
    } else if (sources.length && !sources.some((source) => source.enabled)) {
      ui.noticeStrip.hidden = false;
      setText(ui.noticeStrip, "所有连接都已停用。开启至少一个连接后才能扫描游戏。");
    } else if (app.refreshFailed) {
      ui.noticeStrip.hidden = false;
      setText(ui.noticeStrip, "与 Game Center payload 的连接暂时中断，正在重试。");
    } else {
      ui.noticeStrip.hidden = true;
      setText(ui.noticeStrip, "");
    }
    invalidateFocusCache();
  }

  function currentJob() {
    return app.state && app.state.job ? app.state.job : { state: "idle" };
  }

  function currentJobKey(job) {
    const value = job || currentJob();
    return `${value.state || "idle"}:${value.sequence || value.gameId || ""}:${value.startedAt || 0}:${value.finishedAt || 0}`;
  }

  function setTransferOpen(open) {
    if (open && ui.transferPanel.hidden) rememberFocus("transfer");
    ui.transferPanel.hidden = !open;
    ui.transferButton.classList.toggle("is-open", open);
    ui.transferButton.setAttribute("aria-expanded", open ? "true" : "false");
    invalidateFocusCache();
    if (!open) {
      window.clearTimeout(app.transferAutoHideTimer);
      app.transferAutoHideTimer = 0;
    }
  }

  function dismissTransfer() {
    app.transferDismissedFor = currentJobKey();
    setTransferOpen(false);
    restoreFocus("transfer", ui.transferButton);
  }

  function modalVisible() {
    return !ui.confirmPanel.hidden || !ui.browserPanel.hidden ||
      !ui.editorPanel.hidden || !ui.settingsPanel.hidden ||
      !ui.detailPanel.hidden;
  }

  function transferTargetLabel(target) {
    if (target === "system") return "系统安装";
    if (target === "homebrew") return "安装到 PS5";
    return "安装到主界面";
  }

  function renderTransferQueue(queue) {
    ui.transferQueueList.replaceChildren();
    setText(ui.transferQueueCount, queue.length);
    if (!queue.length) {
      ui.transferQueueList.appendChild(
        element("p", "transfer-queue-empty", "当前没有等待中的安装任务"));
      return;
    }
    for (let index = 0; index < queue.length; index++) {
      const item = queue[index];
      const row = element("div", "transfer-queue-item");
      row.appendChild(element("span", "transfer-queue-index", index + 1));
      const copy = element("span", "transfer-queue-copy");
      copy.appendChild(element("strong", "", item.title || item.gameId || "未命名任务"));
      copy.appendChild(element("small", "",
        [transferTargetLabel(item.target), item.size ? formatBytes(item.size) : ""]
          .filter(Boolean).join(" · ")));
      row.appendChild(copy);
      ui.transferQueueList.appendChild(row);
    }
  }

  function renderTransferHistory(history, job) {
    const currentTerminal = job &&
      ["done", "error", "canceled"].includes(job.state) ? job : null;
    const items = Array.isArray(history) ? history.slice() : [];
    if (currentTerminal && !items.some((item) => currentJobKey(item) === currentJobKey(currentTerminal))) {
      items.unshift(currentTerminal);
    }
    ui.transferHistoryList.replaceChildren();
    setText(ui.transferHistoryCount, items.length);
    ui.clearHistoryButton.hidden = items.length === 0;
    if (!items.length) {
      ui.transferHistoryList.appendChild(
        element("p", "transfer-queue-empty", "还没有传输记录"));
      return;
    }
    for (const item of items) {
      const state = ["done", "error", "canceled"].includes(item.state)
        ? item.state : "done";
      const row = element("div", `transfer-history-item ${state}`);
      const mark = element("span", "transfer-history-state",
        state === "done" ? "✓" : state === "error" ? "!" : "−");
      const copy = element("span", "transfer-queue-copy");
      copy.appendChild(element("strong", "", item.title || item.gameId || "未命名任务"));
      copy.appendChild(element("small", "", [
        state === "done" ? "完成" : state === "error" ? "失败" : "已取消",
        transferTargetLabel(item.target),
        item.bytesTotal ? formatBytes(item.bytesTotal) : "",
        item.finishedAt ? formatFileTime(item.finishedAt) : "",
      ].filter(Boolean).join(" · ")));
      if (item.message) {
        row.title = item.message;
        copy.lastElementChild.title = item.message;
      }
      row.appendChild(mark);
      row.appendChild(copy);
      ui.transferHistoryList.appendChild(row);
    }
  }

  function scheduleTransferAutoHide(key) {
    window.clearTimeout(app.transferAutoHideTimer);
    app.transferAutoHideTimer = window.setTimeout(() => {
      app.transferAutoHideTimer = 0;
      if (currentJobKey() === key && !ui.transferPanel.hidden) {
        const focusInside = ui.transferPanel.contains(document.activeElement);
        app.transferDismissedFor = key;
        setTransferOpen(false);
        if (focusInside) restoreFocus("transfer", ui.transferButton);
      }
    }, 8000);
  }

  function scheduleInstallRefresh() {
    window.clearTimeout(app.transferFollowupTimer);
    app.transferFollowupTimer = window.setTimeout(async () => {
      await refreshState(true);
      window.setTimeout(() => refreshState(true), 1300);
    }, 500);
  }

  function renderJob() {
    if (!app.state) return;
    const job = currentJob();
    const key = currentJobKey(job);
    const queue = Array.isArray(job.queue) ? job.queue : [];
    const running = job.state === "running";
    const done = job.state === "done";
    const failed = job.state === "error";
    const canceled = job.state === "canceled";
    const terminal = done || failed || canceled;
    const changed = !!app.lastJobKey && app.lastJobKey !== key;
    const initialJob = !app.lastJobKey;
    const previousJob = app.renderedJob;
    app.renderedJob = job;

    let progress = Number(job.progress);
    if (!(Number.isFinite(progress) && progress >= 0) && Number(job.bytesTotal) > 0) {
      progress = Math.min(100, Math.max(0,
        Math.round((Number(job.bytesDone) || 0) * 100 / Number(job.bytesTotal))));
    }
    const hasProgress = Number.isFinite(progress) && progress >= 0;
    if (done) progress = 100;
    if (terminal && initialJob) app.transferDismissedFor = key;
    if (!running || changed || job.stage === "取消失败") app.cancelBusy = false;
    if (previousJob && previousJob.stage !== "取消失败" && job.stage === "取消失败") {
      showToast(job.message || "系统取消失败", "error");
    }

    ui.transferButton.classList.toggle("is-running", running);
    ui.transferButton.classList.toggle("is-error", failed);
    ui.transferButton.classList.toggle("is-canceled", canceled);
    ui.transferBadge.className = "transfer-badge";
    ui.transferBadge.hidden = false;
    let badgeText = "";
    if (running) {
      badgeText = hasProgress ? `${Math.max(0, Math.min(100, Math.round(progress)))}%` : "…";
      ui.transferBadge.classList.add("running");
    } else if (failed && app.transferDismissedFor !== key) {
      badgeText = "!";
      ui.transferBadge.classList.add("error");
    } else if (done && app.transferDismissedFor !== key) {
      badgeText = "✓";
      ui.transferBadge.classList.add("done");
    } else if (canceled && app.transferDismissedFor !== key) {
      badgeText = "−";
      ui.transferBadge.classList.add("canceled");
    } else if (queue.length) {
      badgeText = String(queue.length);
      ui.transferBadge.classList.add("queued");
    } else {
      ui.transferBadge.hidden = true;
    }
    setText(ui.transferBadge, badgeText);
    ui.transferButton.setAttribute("aria-label", running
      ? `传输任务，${hasProgress ? `${Math.round(progress)}%` : "正在准备"}`
      : "传输任务");

    setText(ui.transferTitle, job.title ||
      (failed ? "安装失败" : canceled ? "安装已取消" : "传输任务"));
    setText(ui.transferStage, running ? (job.stage || "正在准备") :
      failed ? (job.stage || "安装失败") :
      canceled ? (job.stage || "已取消") :
      done ? (job.stage || "安装完成") : "空闲");
    setText(ui.transferMessage, job.message || (queue.length
      ? `仍有 ${queue.length} 个任务等待安装`
      : "当前没有传输任务"));
    ui.transferStateDot.className = "transfer-state-dot " + (
      running ? "running" : failed ? "error" :
      canceled ? "canceled" : done ? "done" : "idle");

    if (running) {
      setText(ui.transferPercent, hasProgress ? `${Math.round(progress)}%` : "准备中");
    } else if (done) {
      setText(ui.transferPercent, "100%");
    } else if (failed) {
      setText(ui.transferPercent, hasProgress ? `${Math.round(progress)}%` : "失败");
    } else if (canceled) {
      setText(ui.transferPercent, "已取消");
    } else {
      setText(ui.transferPercent, "");
    }
    ui.transferPercent.classList.toggle("canceled", canceled);

    ui.jobProgress.classList.toggle("indeterminate", running && !hasProgress);
    ui.jobProgress.classList.toggle("is-error", failed);
    ui.jobProgress.classList.toggle("is-done", done);
    ui.jobProgress.classList.toggle("is-canceled", canceled);
    ui.jobProgress.style.width = hasProgress
      ? `${Math.max(0, Math.min(100, progress))}%`
      : (failed ? "100%" : canceled ? "0%" : "");

    setText(ui.transferSize, Number(job.bytesTotal) > 0
      ? formatBytes(job.bytesTotal) : "—");
    setText(ui.transferSpeed, running
      ? (Number(job.bytesPerSecond) > 0
        ? `${formatBytes(job.bytesPerSecond)}/s`
        : "计算中…")
      : "—");
    setText(ui.transferDone, Number(job.bytesDone) > 0
      ? formatBytes(job.bytesDone) : (done ? formatBytes(job.bytesTotal) : "—"));
    const queueKey = JSON.stringify(queue);
    if (queueKey !== app.transferQueueKey) {
      app.transferQueueKey = queueKey;
      renderTransferQueue(queue);
      if (topLayer() === ui.transferPanel) invalidateFocusCache();
    }
    const history = Array.isArray(app.state.history) ? app.state.history : [];
    const historyKey = JSON.stringify([history, terminal ? key : ""]);
    if (historyKey !== app.transferHistoryKey) {
      app.transferHistoryKey = historyKey;
      renderTransferHistory(history, job);
      if (topLayer() === ui.transferPanel) invalidateFocusCache();
    }

    const canCancel = running && !!job.gameId && job.stage !== "注册标题";
    const cancelPending = app.cancelBusy || job.cancelPending;
    if (!canCancel && document.activeElement === ui.cancelJobButton) {
      ui.transferClose.focus({ preventScroll: true });
    }
    ui.cancelJobButton.hidden = !canCancel;
    ui.cancelJobButton.disabled = !!cancelPending;
    setText(ui.cancelJobButton, cancelPending ? "正在取消…" : "停止当前安装");

    if (running && !modalVisible() && app.transferDismissedFor !== key) {
      setTransferOpen(true);
    }

    if (changed) {
      app.transferDismissedFor = "";
      if (terminal) {
        if (!modalVisible()) setTransferOpen(true);
        const title = job.title || "安装任务";
        const message = job.message ||
          (done ? "安装任务已完成" : canceled ? "安装已取消" : "安装任务失败");
        if (done) {
          showToast(`${title}：${message}`, "success");
          scheduleTransferAutoHide(key);
          scheduleInstallRefresh();
        } else if (failed) {
          showToast(`${title}：${message}`, "error");
        } else {
          showToast(`${title}：${message}`, "warning");
          scheduleTransferAutoHide(key);
        }
      }
    }
    app.lastJobKey = key;
  }

  function renderPackageList(group) {
    const packages = group.packages;
    ui.detailPackageList.replaceChildren();
    ui.detailPackages.hidden = packages.length === 0;
    if (!packages.length) return;

    const pending = packages.filter((game) => !game.installed).length;
    const unknown = packages.filter((game) =>
      game.installKnown === false || game.installStatus === "unknown").length;
    const incomplete = packages.filter((game) =>
      game.installPartial || game.installStatus === "incomplete").length;
    setText(ui.detailPackagesCount,
      `${packages.length} 个内容包 · ${pending ? `${pending} 个待安装` : "全部已安装"}` +
      (incomplete ? ` · ${incomplete} 个载荷不完整` : "") +
      (unknown ? ` · ${unknown} 个待确认` : ""));

    for (const game of packages) {
      const info = installStatusInfo(game);
      const rowClass = [
        "package-row",
        game.installed ? "is-installed" : "",
        info.tone === "warn" ? "is-warning" : "",
      ].filter(Boolean).join(" ");
      const row = element("div", rowClass);
      const icon = element("span",
        `package-type-icon ${game.pkgKind === "dlc" ? "dlc" : ""} ${game.installed ? "installed" : ""}`.trim(),
        game.pkgKind === "update" ? "UP" : game.pkgKind === "dlc" ? "DLC" : "PKG");
      row.appendChild(icon);
      const copy = element("div", "package-copy");
      const title = [
        packageKindLabel(game),
        game.version ? `v${game.version}` : "",
      ].filter(Boolean).join(" ");
      copy.appendChild(element("strong", "", title));
      const meta = [
        game.title && game.title !== group.title ? game.title : "",
        game.size ? formatBytes(game.size) : "",
        game.contentId || "",
      ]
        .filter(Boolean).join(" · ");
      copy.appendChild(element("small", "", meta || game.path || ""));
      copy.lastElementChild.title = meta || game.path || "";
      row.appendChild(copy);
      row.appendChild(element("span", `package-status ${info.tone}`.trim(), info.label));

      const button = element("button", "secondary-button compact");
      button.type = "button";
      button.dataset.packageId = game.id;
      button.hidden = info.disabled;
      row.appendChild(button);
      ui.detailPackageList.appendChild(row);
    }
  }

  function renderDetail(force) {
    if (!app.selectedGameId || ui.detailPanel.hidden) return;
    const active = document.activeElement;
    const activeInDetail = ui.detailPanel.contains(active);
    const detailKey = [
      Number(app.state && app.state.catalogRevision) || 0,
      app.selectedGameId,
    ].join(":");
    const group = findGameGroup(app.selectedGameId);
    if (!group) {
      closeDetail();
      return;
    }
    const game = group.primary;
    const sizeInfo = gameSizeInfo(group, game);
    if (!force && detailKey === app.detailKey) {
      refreshDetailSpace(group, game, sizeInfo);
      return;
    }
    app.detailKey = detailKey;
    const art = group.pic0 || group.pic1 || group.icon;
    ui.detailArt.style.backgroundImage = art ? `url("${art}")` : "";
    const hasWideArt = !!(group.pic0 || group.pic1);
    ui.detailArt.classList.toggle("has-wide-art", hasWideArt);
    ui.detailArt.classList.toggle("icon-only-art", !hasWideArt && !!group.icon);
    ui.detailCover.replaceChildren();
    const cover = createCover(group, "detail-cover-inner");
    cover.className = "";
    cover.style.width = "100%";
    cover.style.height = "100%";
    cover.style.position = "relative";
    cover.style.overflow = "hidden";
    ui.detailCover.appendChild(cover);

    const format = detailFormatInfo(game, group);
    ui.detailFormat.className = `detail-format-line ${format.tone}`;
    const formatIcon = element("span", "detail-format-icon");
    formatIcon.innerHTML = detailFormatIcon(format.kind);
    ui.detailFormat.replaceChildren(formatIcon);
    const formatCopy = element("span", "detail-format-copy");
    const formatHead = element("span", "detail-format-head");
    formatHead.appendChild(element("strong", "", format.name));
    const tokenTone = format.kind === "package"
      ? "system"
      : game.packing === "plain" ? "plain" : "packing";
    formatHead.appendChild(element("span",
      `detail-format-token ${tokenTone}`,
      format.token));
    formatCopy.appendChild(formatHead);
    formatCopy.appendChild(element("small", "",
      [format.detail, group.sourceName ? `来自 ${group.sourceName}` : ""]
        .filter(Boolean).join(" · ")));
    ui.detailFormat.appendChild(formatCopy);

    setText(ui.detailTitle, group.title || group.titleId || "未命名游戏");
    setText(ui.detailSubtitle, [
      group.titleId || "无 Title ID",
      group.version ? `版本 ${group.version}` : "",
    ].filter(Boolean).join(" · "));
    setText(ui.detailPath, `路径：${game.path || "/"}`);
    ui.detailPath.title = game.path || "/";

    const compressed = game.type === "folder" && game.packing !== "plain";
    const installed = !!group.installed;
    const launchable = !!group.launchable;
    const installedMeta = [
      launchable ? "可直接启动" : "",
      game.installedVersion ? `系统版本 ${game.installedVersion}` : "",
      game.libraryCopy ? "已注册到主界面" : "",
      installed && !game.installedVersion && !game.libraryCopy ? "系统安装状态已确认" : "",
    ].filter(Boolean);
    ui.detailInstalledState.hidden = !installed;
    setText(ui.detailInstalledMeta,
      installedMeta.join(" · ") || "基础游戏已安装，无需再次安装");
    setText(ui.detailActionTitle,
      launchable ? "启动与内容" : installed ? "游戏内容" : "安装与内容");
    ui.detailInstallStatus.className = "detail-install-status " + (
      installed ? "good" :
      group.primaryIsContent || group.baseInstalled === false ? "warn" :
      compressed && game.homebrewCopy ? "muted" : "");
    setText(ui.detailInstallStatus,
      installed ? "已安装" :
      group.primaryIsContent ? (group.baseInstalled ? "内容包" : "需基础游戏") :
      compressed && game.homebrewCopy ? "本机已有" : "可安装");

    ui.detailTechNote.hidden = true;
    setText(ui.detailTechNote, "");
    ui.launchButton.hidden = !launchable;
    ui.launchButton.disabled = !!app.launchBusy;
    ui.launchButton.classList.toggle("is-launching", !!app.launchBusy);
    setText(ui.launchButtonText, app.launchBusy ? "正在启动…" : "启动游戏");
    ui.launchButton.dataset.defaultLabel = "启动游戏";
    ui.installButton.disabled = false;
    ui.installButton.hidden = installed;

    if (group.primaryIsContent) {
      ui.installButton.hidden = true;
      ui.installButton.disabled = true;
      setText(ui.detailHint, group.baseInstalled
        ? "NAS 上只有该游戏的内容包。请在下方选择要安装的补丁或 DLC；系统安装器会通过本地 HTTP 流读取 PKG。"
        : "NAS 上只有该游戏的内容包，且系统尚未检测到基础游戏。请先安装基础游戏，再从下方列表选择补丁或 DLC。");
    } else if (compressed) {
      setText(ui.installButton, game.homebrewCopy ? "本机已有副本" : "安装到 PS5");
      setText(ui.detailHint,
        installed
          ? "基础游戏已安装，无需重复安装。"
          : `${game.packing.toUpperCase()} 压缩文件夹将安装到 PS5 本机，不会注册到主界面。`);
      ui.detailTechNote.hidden = installed;
      setText(ui.detailTechNote,
        `说明：内容会安装到 /data/homebrew/${group.titleId || "<Title ID>"}；如需从主界面启动，可使用 Kraken Mount 扫描挂载。`);
    } else if (game.type === "folder") {
      setText(ui.installButton, game.libraryCopy ? "重新注册到主界面" : "安装到 PS5");
      setText(ui.detailHint, installed
        ? "基础游戏已安装，无需重复安装；补丁和 DLC 可在下方单独管理。"
        : `当前未检测到基础游戏。游戏会安装到 /data/nasgames/library/${group.titleId || "<Title ID>"} 并注册到主界面；补丁和 DLC 可在下方单独选择安装。`);
    } else {
      const mainInfo = installStatusInfo(game);
      setText(ui.installButton, mainInfo.disabled ? mainInfo.label :
        (game.pkgKind === "update" ? "安装补丁到 PS5" :
          game.pkgKind === "dlc" ? "安装 DLC 到 PS5" : "安装到 PS5"));
      ui.installButton.disabled = mainInfo.disabled;
      const updateHint = game.pkgKind === "update"
        ? "当前卡片主资源是补丁包。"
        : game.pkgKind === "dlc"
          ? "当前卡片主资源是 DLC。"
          : "";
      setText(ui.detailHint, [installed ? "基础游戏已安装，无需重复安装。" : "", updateHint,
        "同一游戏的补丁和 DLC 已合并在下方列表，系统安装器会通过本地 HTTP 流读取 NAS 上的 PKG。"]
        .filter(Boolean).join(" "));
    }
    if (installed && launchable) {
      setText(ui.detailHint,
        "基础游戏已安装，可直接启动；启动后会退出当前网页。补丁和 DLC 可在下方单独管理。");
    } else if (installed) {
      setText(ui.detailHint,
        "系统已确认基础游戏安装，但当前记录缺少可启动的 Title ID。");
    }
    ui.installButton.dataset.defaultLabel = ui.installButton.textContent;
    app.detailSpaceKey = "";
    requestGameSize(group, game);
    renderPackageList(group);
    refreshDetailSpace(group, game, sizeInfo);
    invalidateFocusCache();
    if (activeInDetail && !active.isConnected) {
      const replacement = active.dataset.packageId && ui.detailPackageList.querySelector(
        `[data-package-id="${CSS.escape(active.dataset.packageId)}"]:not([disabled])`);
      window.setTimeout(() => {
        if (topLayer() !== ui.detailPanel) return;
        if (replacement && replacement.isConnected) replacement.focus({ preventScroll: true });
        else focusDetailAction();
      }, 0);
    }
  }

  function refreshDetailSpace(group, game, sizeInfo) {
    const storage = app.state && app.state.storage;
    const key = JSON.stringify([storage && storage.system, storage && storage.library,
      storage && storage.homebrew, sizeInfo]);
    if (key === app.detailSpaceKey) return;
    app.detailSpaceKey = key;

    const focused = document.activeElement;
    const registrationOnly = game.type === "folder" && game.packing === "plain" && game.libraryCopy;
    const alreadyCopied = game.type === "folder" && game.packing !== "plain" && game.homebrewCopy;
    const insufficient = renderDetailStorage(game, sizeInfo, registrationOnly);
    const loading = game.type === "folder" && sizeInfo.status === "loading" && !registrationOnly;
    const disabled = ui.installButton.hidden || alreadyCopied || insufficient || loading ||
      (game.type === "pkg" && installStatusInfo(game).disabled);
    if (ui.installButton.disabled !== disabled) ui.installButton.disabled = disabled;
    setText(ui.installButton, insufficient ? "可用空间不足" : loading ? "正在统计大小" :
      ui.installButton.dataset.defaultLabel);
    ui.installButton.title = insufficient ? "目标盘可用空间不足" : "";

    const system = storage && storage.system;
    const buttons = ui.detailPackageList.querySelectorAll("[data-package-id]");
    for (const [index, item] of group.packages.entries()) {
      const button = buttons[index];
      const noSpace = !!(system && system.known && item.size > Number(system.free));
      const info = installStatusInfo(item);
      const unavailable = info.disabled || noSpace;
      if (button.disabled !== unavailable) button.disabled = unavailable;
      setText(button, info.disabled ? info.label : noSpace ? "空间不足" : `安装${packageKindLabel(item)}`);
      button.title = noSpace ? "系统可用空间不足" : "";
      button.parentElement.classList.toggle("no-action", unavailable);
    }
    if (focused && ui.detailPanel.contains(focused) && (focused.disabled || focused.hidden) &&
        topLayer() === ui.detailPanel) focusDetailAction();
  }

  function renderDetailStorage(game, sizeInfo, registrationOnly) {
    const storage = app.state && app.state.storage;
    const key = game.type === "pkg"
      ? "system"
      : game.packing !== "plain" ? "homebrew" : "library";
    const info = storage && storage[key];
    ui.detailStorage.hidden = !info || !info.known;
    if (ui.detailStorage.hidden) return false;
    const free = Number(info.free) || 0;
    const total = Number(info.total) || 0;
    const required = Number(sizeInfo.size) || 0;
    const insufficient = !registrationOnly && required > 0 && required > free;
    const label = game.type === "pkg"
      ? "PS5 系统盘"
      : game.packing !== "plain" ? "本机应用目录" : "Game Center 本地库";
    const sizeText = sizeInfo.status === "known" || required > 0
      ? formatBytes(required)
      : sizeInfo.status === "error" ? "统计失败" : "统计中…";
    const remainingText = registrationOnly ? formatBytes(free) : insufficient
      ? formatBytes(required - free)
      : required > 0 ? formatBytes(Math.max(0, free - required)) : "—";

    const metric = (name, value, tone) => {
      const cell = element("div", `detail-storage-metric ${tone || ""}`.trim());
      cell.appendChild(element("small", "", name));
      cell.appendChild(element("strong", "", value));
      return cell;
    };

    ui.detailStorage.className = "detail-storage" + (insufficient ? " warn" : "");
    ui.detailStorage.replaceChildren();
    const head = element("div", "detail-storage-head");
    head.appendChild(element("strong", "", "空间检查"));
    head.appendChild(element("span", "", label));
    ui.detailStorage.appendChild(head);
    const grid = element("div", "detail-storage-grid");
    grid.appendChild(metric("文件大小", sizeText,
      sizeInfo.status === "error" ? "warn" : ""));
    grid.appendChild(metric("可用空间", formatBytes(free), ""));
    grid.appendChild(metric(insufficient ? "还差空间" : "预计剩余", remainingText,
      required > 0 ? (insufficient ? "warn" : "good") : ""));
    ui.detailStorage.appendChild(grid);

    let note;
    if (registrationOnly) {
      note = "使用已经复制的本地文件注册游戏。";
    } else if (sizeInfo.status === "error") {
      note = "无法读取源文件大小，安装时服务端会再次检查。";
    } else if (sizeInfo.status !== "known" && required === 0) {
      note = "正在统计源文件大小，完成后会自动更新空间判断。";
    } else if (insufficient) {
      note = "目标盘空间不足，安装按钮已禁用。";
    } else if (game.type === "pkg") {
      note = "按 PKG 文件大小估算；实际安装占用由系统决定，可能更高。";
    } else {
      note = "按远端文件夹完整复制估算。";
    }
    if (total > 0) note += ` 目标盘总计 ${formatBytes(total)}。`;
    ui.detailStorage.appendChild(element("p", "detail-storage-note", note));
    return insufficient;
  }

  function focusDetailAction() {
    const packageButton = ui.detailPackageList.querySelector("button:not([disabled])");
    const target = !ui.launchButton.hidden && !ui.launchButton.disabled
      ? ui.launchButton
      : !ui.installButton.hidden && !ui.installButton.disabled
      ? ui.installButton
      : packageButton || ui.detailClose;
    target.focus({ preventScroll: true });
  }

  function render() {
    renderHeaderState();
    renderLibrary(false);
    renderJob();
    renderDetail();
    if (!ui.settingsPanel.hidden) renderSettingsOverview();
  }

  async function refreshState(silent) {
    if (app.refreshBusy) return;
    app.refreshBusy = true;
    try {
      const next = await api("/api/state");
      if (app.state && (app.state.version !== next.version ||
          app.state.server.instance !== next.server.instance)) {
        app.groupsRevision = -1;
        app.librarySignature = "";
        app.detailKey = "";
        app.headerKey = "";
        app.lastJobKey = "";
        app.transferDismissedFor = "";
        app.cancelBusy = false;
        app.launchBusy = false;
        app.scanPending = false;
        app.scanSequence = null;
        app.sizeStates = Object.create(null);
        app.preloadedArt.clear();
      }
      if (app.state && app.state.catalogConfigGeneration !== next.catalogConfigGeneration) {
        app.sizeStates = Object.create(null);
        app.preloadedArt.clear();
      }
      app.state = next;
      app.refreshFailed = false;
      if (app.scanPending && app.scanSequence !== null && !app.state.scan.busy &&
          Number(app.state.scan.sequence) >= app.scanSequence) {
        app.scanPending = false;
        app.scanSequence = null;
      }
      render();
    } catch (error) {
      app.refreshFailed = true;
      if (!silent) showToast(`无法读取状态：${error.message}`, "error");
      renderHeaderState();
    } finally {
      app.refreshBusy = false;
    }
  }

  function needsFastRefresh() {
    const job = currentJob();
    const scan = app.state && app.state.scan ? app.state.scan : {};
    return !!app.scanPending || !!scan.busy || job.state === "running" ||
      (Array.isArray(job.queue) && job.queue.length > 0);
  }

  function scheduleStatusPoll(delay) {
    window.clearTimeout(app.statusTimer);
    app.statusTimer = window.setTimeout(async () => {
      app.statusTimer = 0;
      await refreshState(true);
      scheduleStatusPoll(needsFastRefresh() ? 900 : 2200);
    }, delay === undefined ? (needsFastRefresh() ? 900 : 2200) : delay);
  }

  async function requestScan() {
    app.scanPending = true;
    app.scanSequence = null;
    ui.scanButton.classList.add("spinning");
    try {
      const result = await api("/api/scan", { method: "POST" });
      app.scanSequence = Number(result.sequence) || 0;
      showToast("已请求重新扫描");
      scheduleStatusPoll(420);
    } catch (error) {
      app.scanPending = false;
      app.scanSequence = null;
      showToast(`扫描失败：${error.message}`, "error");
    }
    renderHeaderState();
  }

  function invalidateFocusCache() {
    app.focusCache = null;
  }

  function rememberFocus(layer) {
    const active = document.activeElement;
    app.focusReturn[layer] = active && active !== document.body ? active : null;
  }

  function restoreFocus(layer, fallback) {
    const target = app.focusReturn[layer];
    app.focusReturn[layer] = null;
    window.setTimeout(() => {
      const scope = topLayer();
      const gameId = target && target.dataset.gameId;
      const card = gameId && scope.querySelector(`[data-game-id="${CSS.escape(gameId)}"]`);
      const next = [target, card, fallback].find((node) =>
        node && scope.contains(node) && !node.closest("[hidden]") && !node.disabled);
      if (next) next.focus({ preventScroll: true });
      invalidateFocusCache();
    }, 20);
  }

  function openDetail(gameId) {
    rememberFocus("detail");
    setTransferOpen(false);
    app.selectedGameId = gameId;
    const group = findGameGroup(gameId);
    if (group) {
      const measured = app.sizeStates[group.primary.id];
      if (measured && measured.status === "error") delete app.sizeStates[group.primary.id];
      preloadDetailArt(group);
    }
    ui.detailPanel.hidden = false;
    app.detailKey = "";
    renderDetail(true);
    invalidateFocusCache();
    window.setTimeout(() => {
      if (!ui.detailPanel.hidden) focusDetailAction();
    }, 30);
  }

  function closeDetail(restore = true) {
    app.selectedGameId = null;
    app.detailKey = "";
    ui.detailPanel.hidden = true;
    invalidateFocusCache();
    window.setTimeout(renderJob, 0);
    if (restore) restoreFocus("detail", ui.search);
  }

  function askConfirm(title, message, okLabel, callback, showCancel = true,
                      cancelLabel = "取消") {
    rememberFocus("confirm");
    app.confirmCallback = callback;
    setText(ui.confirmTitle, title);
    setText(ui.confirmMessage, message);
    setText(ui.confirmOk, okLabel || "确认");
    setText(ui.confirmCancel, cancelLabel || "取消");
    ui.confirmOk.disabled = false;
    ui.confirmCancel.hidden = !showCancel;
    ui.confirmPanel.hidden = false;
    invalidateFocusCache();
    window.setTimeout(() => ui.confirmOk.focus({ preventScroll: true }), 20);
  }

  function closeConfirm() {
    if (app.confirmBusy) return;
    app.confirmCallback = null;
    ui.confirmPanel.hidden = true;
    invalidateFocusCache();
    restoreFocus("confirm", ui.confirmCancel);
  }

  function beginInstall(game, target) {
    if (game.type === "folder" && game.packing !== "plain" && target === "library") {
      showToast("压缩文件夹会安装到 PS5 本机，请重新打开详情后安装", "error");
      return;
    }
    if (!target) return;
    const preflight = installPreflight(game, target);
    if (!preflight.registrationOnly && preflight.required === 0 && preflight.sizeStatus === "loading") {
      showToast("正在统计游戏大小，请稍候");
      return;
    }
    if (preflight.insufficient) {
      askConfirm(
        "空间不足",
        `“${game.title}”需要 ${formatBytes(preflight.required)}。\n` +
          `${preflight.targetInfo.label}（${preflight.targetInfo.path}）当前可用 ` +
          `${formatBytes(preflight.free)}，还差 ${formatBytes(preflight.shortfall)}。\n\n` +
          "请清理目标盘空间后重试。",
        "知道了",
        async () => {},
        false,
      );
      return;
    }
    const sizeLabel = game.type === "pkg" ? "PKG 文件大小" : "远端文件总大小";
    const spaceSummary = preflight.registrationOnly
      ? "\n\n将使用现有的本地副本，直接重新注册。"
      : preflight.required > 0 && preflight.freeKnown
      ? `\n\n${sizeLabel}：${formatBytes(preflight.required)}` +
        `\n目标盘可用：${formatBytes(preflight.free)}` +
        `\n按文件大小估算，安装后剩余：${formatBytes(preflight.remaining)}`
      : preflight.sizeStatus === "error"
        ? "\n\n暂时无法统计资源大小，服务器会在开始安装前再次检查空间。"
        : "\n\n暂时无法读取目标盘空间，服务器会在开始安装前再次检查。";

    let title = "安装游戏";
    let message = "";
    let okLabel = "开始";
    let started = "安装任务已提交";
    if (target === "system") {
      title = game.pkgKind === "update"
        ? "安装游戏补丁"
        : game.pkgKind === "dlc" ? "安装 DLC" : "安装 PKG";
      message = `系统安装器将读取 NAS 上的“${game.title}”并安装${game.version ? ` v${game.version}` : ""}。` +
        (game.pkgKind === "update" || game.pkgKind === "dlc"
          ? "该内容需要同 Title ID 的基础游戏已安装。" : "");
      okLabel = "开始安装";
      started = `${game.pkgKind === "dlc" ? "DLC" : game.pkgKind === "update" ? "补丁" : "PKG"}安装任务已提交`;
    } else if (target === "library") {
      title = "安装到 PS5 主界面";
      message = preflight.registrationOnly ? `重新注册“${game.title}”到 PS5 主界面。` :
        `将把“${game.title}”安装到 /data/nasgames/library/${game.titleId}，再注册并挂载到主界面。`;
      okLabel = preflight.registrationOnly ? "重新注册" : "开始安装";
      started = preflight.registrationOnly ? "主界面注册任务已提交" : "主界面安装任务已提交";
    } else if (target === "homebrew") {
      title = "安装到 PS5";
      message = `将把“${game.title}”安装到 PS5 本机的 /data/homebrew/${game.titleId}。该目录不会自动注册到主界面，目标已存在时不会覆盖。`;
      okLabel = "开始安装";
      started = "安装到 PS5 的任务已提交";
    }
    message += spaceSummary;
    if (target === "system") {
      message += "\nPKG 安装后的实际占用可能高于文件大小，并额外使用临时空间。";
    }
    message += "\n\n继续安装吗？";
    askConfirm(title, message, okLabel, async () => {
      await api("/api/install", { method: "POST", body: { gameId: game.id, target } });
      app.transferDismissedFor = "";
      closeDetail();
      showToast(started);
      scheduleStatusPoll(300);
    });
  }

  function isLauncherMode() {
    return new URLSearchParams(window.location.search).get("launcher") === "1" ||
      /playstation/i.test(navigator.userAgent || "");
  }

  function resetLaunchControl(gameId) {
    app.launchBusy = false;
    const selected = findGameGroup(app.selectedGameId);
    if (!ui.detailPanel.hidden && selected && selected.primary.id === gameId) {
      ui.launchButton.disabled = false;
      ui.launchButton.classList.remove("is-launching");
      setText(ui.launchButtonText, "启动游戏");
    }
  }

  function finishLaunchScreen(warn) {
    const gameId = app.launchScreenGameId;
    window.clearTimeout(app.launchResetTimer);
    app.launchResetTimer = 0;
    app.launchScreenActive = false;
    app.launchScreenGameId = "";
    ui.launchScreen.hidden = true;
    resetLaunchControl(gameId);
    if (warn) {
      showToast("启动请求未完成，请检查运行日志", "warning");
    }
  }

  function beginLaunchScreen(title, gameId) {
    window.clearTimeout(app.launchResetTimer);
    app.launchScreenActive = true;
    app.launchScreenGameId = gameId;
    setText(ui.launchScreenTitle, title);
    ui.launchScreen.hidden = false;
    app.launchResetTimer = window.setTimeout(() => {
      app.launchResetTimer = 0;
      if (document.hidden) return;
      finishLaunchScreen(true);
    }, 12000);
  }

  async function launchGame() {
    const group = findGameGroup(app.selectedGameId);
    if (!group || !group.launchable || app.launchBusy) return;
    const gameId = group.primary.id;
    const title = group.title || group.titleId || "该游戏";

    app.launchBusy = true;
    ui.launchButton.disabled = true;
    ui.launchButton.classList.add("is-launching");
    setText(ui.launchButtonText, "正在启动…");
    try {
      const result = await api("/api/launch", {
        method: "POST",
        body: { gameId },
      });
      if (isLauncherMode()) {
        beginLaunchScreen(title, gameId);
        return;
      }
      showToast(result.message || `正在启动“${title}”`, "success");
    } catch (error) {
      showToast(`无法启动“${title}”：${error.message}`, "error");
    } finally {
      if (!app.launchScreenActive) resetLaunchControl(gameId);
    }
  }

  function cancelInstallJob() {
    const job = currentJob();
    if (job.state !== "running" || !job.gameId) return;
    askConfirm(
      "停止安装",
      `停止“${job.title || "当前任务"}”？\n\n` +
        "已写入的临时文件会清理；如果安装已交给系统，会尝试请求系统取消。\n\n" +
        "确定要停止当前任务吗？",
      "停止安装",
      async () => {
        app.cancelBusy = true;
        renderJob();
        try {
          const result = await api("/api/cancel", {
            method: "POST",
            body: { gameId: job.gameId },
          });
          showToast(result.message || "正在取消当前安装", "warning");
          scheduleStatusPoll(250);
        } catch (error) {
          app.cancelBusy = false;
          renderJob();
          throw new Error(`取消失败：${error.message}`);
        }
      },
      true,
      "继续安装",
    );
  }

  function clearTransferHistory() {
    askConfirm(
      "清除传输记录",
      "清除所有已完成、失败和取消的任务记录？\n\n" +
        "运行中的任务和等待队列不会受到影响。",
      "清除记录",
      async () => {
        await api("/api/history/clear", { method: "POST" });
        app.transferHistoryKey = "";
        app.lastJobKey = "";
        app.transferDismissedFor = "";
        await refreshState(true);
        showToast("传输记录已清除", "success");
        window.setTimeout(() => {
          if (!ui.transferPanel.hidden) {
            ui.transferClose.focus({ preventScroll: true });
          }
        }, 60);
      },
      true,
      "保留记录",
    );
  }

  async function openSettings() {
    rememberFocus("settings");
    if (!ui.detailPanel.hidden) closeDetail(false);
    setTransferOpen(false);
    try {
      app.settings = await api("/api/settings");
      app.settingsOverviewKey = "";
      fillSettings();
      ui.settingsPanel.hidden = false;
      invalidateFocusCache();
      switchSettingsTab("overview");
      window.setTimeout(() => {
        const active = ui.settingsNav.querySelector(".settings-nav-item.is-active");
        (active || ui.settingsClose).focus({ preventScroll: true });
      }, 20);
    } catch (error) {
      showToast(`读取设置失败：${error.message}`, "error");
    }
  }

  function fillSettings() {
    const settings = app.settings;
    ui.settingPort.value = settings.port || 8098;
    ui.settingInterval.value = settings.scan_interval_seconds || 300;
    ui.settingDepth.value = settings.max_scan_depth === undefined ? 4 : settings.max_scan_depth;
    ui.settingAutoScan.checked = !!settings.auto_scan;
    ui.settingOpenBrowser.checked = !!settings.open_browser_on_start;
    ui.settingKeepAwake.checked = settings.keep_awake !== false;
    setText(ui.settingsHint, "修改连接后会自动重新扫描。");
    renderConnections();
    renderSettingsOverview();
  }

  function sourceRuntimeState(source) {
    const stateSource = findStateSource(source.id);
    if (!source.enabled) return { label: "已停用", tone: "muted" };
    if (!stateSource) return { label: "等待扫描", tone: "" };
    if (stateSource.status === "online") return { label: "在线", tone: "online" };
    if (stateSource.status === "error") {
      return { label: "连接异常", tone: "error", error: stateSource.error || "" };
    }
    return { label: "等待连接", tone: "" };
  }

  function renderSettingsOverview() {
    if (!app.settings) return;
    const sources = app.settings.sources || [];
    const key = JSON.stringify({
      revision: Number(app.state && app.state.catalogRevision) || 0,
      settings: sources.map((source) => [
        source.id, source.name, source.type, source.enabled, source.host,
        source.share, source.localRoot, source.roots,
      ]),
      runtime: (app.state && app.state.sources || []).map((source) =>
        [source.id, source.status, source.error]),
    });
    if (key === app.settingsOverviewKey) return;
    app.settingsOverviewKey = key;
    const enabled = sources.filter((source) => source.enabled);
    const online = enabled.filter((source) => {
      const state = sourceRuntimeState(source);
      return state.tone === "online";
    }).length;
    const failed = enabled.filter((source) => sourceRuntimeState(source).tone === "error");

    setText(ui.settingsConnectionCount, `${online}/${enabled.length}`);
    if (!sources.length) {
      setText(ui.settingsConnectionDetail, "尚未配置连接");
    } else if (!enabled.length) {
      setText(ui.settingsConnectionDetail, "全部连接均已停用");
    } else if (failed.length) {
      setText(ui.settingsConnectionDetail, `${failed.length} 个连接需要检查`);
    } else if (online === enabled.length) {
      setText(ui.settingsConnectionDetail, "所有启用的连接在线");
    } else {
      setText(ui.settingsConnectionDetail, "等待下一次扫描确认");
    }

    const groups = gameGroups();
    const installed = groups.filter((group) => group.installed).length;
    setText(ui.settingsGameCount, groups.length);
    setText(ui.settingsGameDetail, groups.length
      ? `${installed} 已安装 · ${groups.length - installed} 未安装`
      : "等待首次扫描");

    const pendingGroups = groups.filter((group) =>
      group.incompleteContent ||
      (group.installed && (group.pendingPatches || group.pendingDlc)));
    const attention = pendingGroups.length + failed.length;
    setText(ui.settingsNavConnections, `${sources.length} 个来源`);
    setText(ui.connectionsToolbarCount, `${sources.length} 个来源`);
    setText(ui.settingsAttentionCount, attention);
    if (!attention) {
      setText(ui.settingsAttentionDetail, "没有待办项目");
    } else {
      const parts = [];
      if (pendingGroups.length) {
        parts.push(`${pendingGroups.length} 款游戏有待处理内容`);
      }
      if (failed.length) parts.push(`${failed.length} 个连接异常`);
      setText(ui.settingsAttentionDetail, parts.join(" · "));
    }
    ui.settingsAttentionList.replaceChildren();
    ui.settingsAttentionList.hidden = attention === 0;
    if (attention) {
      const head = element("div", "settings-attention-list-head");
      head.appendChild(element("strong", "", "具体项目"));
      head.appendChild(element("span", "", `${attention} 项`));
      ui.settingsAttentionList.appendChild(head);
      for (const group of pendingGroups.slice(0, 6)) {
        const reasons = [];
        if (group.installed && group.pendingPatches) {
          reasons.push(`${group.pendingPatches} 个补丁可安装`);
        }
        if (group.installed && group.pendingDlc) {
          reasons.push(`${group.pendingDlc} 个 DLC 可安装`);
        }
        if (group.incompleteContent) reasons.push(`${group.incompleteContent} 个载荷不完整`);
        if (!group.installed && (group.pendingPatches || group.pendingDlc)) {
          reasons.push("基础游戏尚未安装");
        }
        const row = element("div", "settings-attention-item");
        row.appendChild(element("span", "settings-attention-mark", "!"));
        const copy = element("span", "settings-attention-copy");
        copy.appendChild(element("strong", "", group.title || group.titleId || "未命名游戏"));
        copy.appendChild(element("small", "", reasons.join(" · ")));
        row.appendChild(copy);
        ui.settingsAttentionList.appendChild(row);
      }
      for (const source of failed.slice(0, 4)) {
        const row = element("div", "settings-attention-item error");
        row.appendChild(element("span", "settings-attention-mark", "×"));
        const copy = element("span", "settings-attention-copy");
        copy.appendChild(element("strong", "", source.name || source.id));
        copy.appendChild(element("small", "",
          sourceRuntimeState(source).error || "连接异常，请检查地址和账号"));
        row.appendChild(copy);
        ui.settingsAttentionList.appendChild(row);
      }
      const shown = Math.min(6, pendingGroups.length) + Math.min(4, failed.length);
      if (attention > shown) {
        ui.settingsAttentionList.appendChild(element(
          "p", "settings-attention-more", `另有 ${attention - shown} 项未展开`));
      }
    }

    if (failed.length) {
      ui.settingsHealth.hidden = false;
      ui.settingsHealth.className = "settings-health error";
      setText(ui.settingsHealth,
        `${failed[0].name || failed[0].id}：${sourceRuntimeState(failed[0]).error || "连接失败，请测试并检查配置。"}`);
    } else if (!sources.length) {
      ui.settingsHealth.hidden = false;
      ui.settingsHealth.className = "settings-health info";
      setText(ui.settingsHealth, "添加 SMB、FTP 或本地目录后，Game Center 会自动扫描并建立海报墙。");
    } else if (!enabled.length) {
      ui.settingsHealth.hidden = false;
      ui.settingsHealth.className = "settings-health warn";
      setText(ui.settingsHealth, "所有连接均已停用，启用至少一个连接后才能刷新游戏库。");
    } else {
      ui.settingsHealth.hidden = true;
      setText(ui.settingsHealth, "");
    }
  }

  function closeSettings() {
    stopLogPolling();
    ui.settingsPanel.hidden = true;
    app.settings = null;
    app.settingsOverviewKey = "";
    invalidateFocusCache();
    window.setTimeout(renderJob, 0);
    restoreFocus("settings", ui.settingsButton);
  }

  function renderConnections() {
    ui.connectionsList.replaceChildren();
    if (!app.settings || !app.settings.sources.length) {
      const empty = element("div", "empty-state connection-empty");
      empty.appendChild(element("h2", "", "尚未添加连接"));
      empty.appendChild(element("p", "", "使用右上角的 SMB、FTP 或本地按钮添加一个游戏来源。"));
      ui.connectionsList.appendChild(empty);
      return;
    }

    for (const source of app.settings.sources) {
      const runtime = sourceRuntimeState(source);
      const card = element("article",
        `connection-card type-${source.type} ${source.enabled ? "" : "disabled"}`.trim());
      card.dataset.id = source.id;

      const head = element("div", "connection-head");
      const identity = element("div", "connection-identity");
      identity.appendChild(element("div", "connection-icon",
        source.type === "local" ? "L" : source.type));
      const copy = element("div", "connection-copy");
      const nameRow = element("div", "connection-name-row");
      nameRow.appendChild(element("strong", "", source.name || source.id));
      nameRow.appendChild(element("span", "protocol-tag", sourceTypeLabel(source.type)));
      copy.appendChild(nameRow);
      copy.appendChild(element("code", "connection-endpoint", sourceEndpoint(source)));
      identity.appendChild(copy);
      head.appendChild(identity);

      const toggleLabel = element("label", "connection-toggle");
      toggleLabel.title = source.enabled ? "停用此连接" : "启用此连接";
      const toggle = document.createElement("input");
      toggle.type = "checkbox";
      toggle.className = "switch mini-switch";
      toggle.checked = !!source.enabled;
      toggle.dataset.action = "toggle";
      toggle.dataset.id = source.id;
      toggle.setAttribute("aria-label", `启用 ${source.name || source.id}`);
      toggleLabel.appendChild(toggle);
      head.appendChild(toggleLabel);
      card.appendChild(head);

      const meta = element("div", "connection-meta");
      const status = element("span", `status-chip ${runtime.tone}`.trim());
      status.appendChild(element("i"));
      status.appendChild(document.createTextNode(runtime.label));
      meta.appendChild(status);
      const roots = Array.isArray(source.roots) && source.roots.length
        ? source.roots
        : ["/"];
      meta.appendChild(element("span", "connection-meta-item",
        `${roots.length} 个扫描目录`));
      card.appendChild(meta);

      const rootPreview = element("div", "connection-root-preview");
      for (const root of roots.slice(0, 2)) {
        rootPreview.appendChild(element("span", "", root));
      }
      if (roots.length > 2) {
        rootPreview.appendChild(element("span", "more", `+${roots.length - 2}`));
      }
      card.appendChild(rootPreview);

      if (runtime.error) {
        const error = element("p", "connection-error", runtime.error);
        error.title = runtime.error;
        card.appendChild(error);
      }

      const actions = element("div", "connection-actions");
      const edit = element("button", "connection-action primary", "编辑配置");
      edit.type = "button";
      edit.dataset.action = "edit";
      edit.dataset.id = source.id;
      actions.appendChild(edit);
      const test = element("button", "connection-action", "测试连接");
      test.type = "button";
      test.dataset.action = "test";
      test.dataset.id = source.id;
      actions.appendChild(test);
      const remove = element("button", "connection-action danger", "删除");
      remove.type = "button";
      remove.dataset.action = "delete";
      remove.dataset.id = source.id;
      actions.appendChild(remove);
      card.appendChild(actions);
      ui.connectionsList.appendChild(card);
    }
    renderSettingsOverview();
  }

  function defaultPort(type) {
    if (type === "smb") return 445;
    if (type === "ftp") return 21;
    return 0;
  }

  function newSource(type) {
    const stamp = Date.now().toString(36);
    const random = Math.random().toString(36).slice(2, 6);
    const label = type === "smb" ? "SMB 连接" : type === "ftp" ? "FTP 连接" : "本地游戏目录";
    return {
      id: `${type}-${stamp}${random}`.slice(0, 28),
      name: label,
      type,
      enabled: true,
      host: "",
      port: defaultPort(type),
      share: "",
      username: "",
      domain: "",
      localRoot: type === "local" ? "/data" : "/",
      roots: ["/"],
      has_password: false,
      password: "",
    };
  }

  function editorTypeInfo(type) {
    if (type === "ftp") {
      return {
        icon: "FTP",
        title: "FTP 网络连接",
        hint: "填写服务器地址与登录凭据，再添加要扫描的远端目录。",
      };
    }
    if (type === "local") {
      return {
        icon: "L",
        title: "本地游戏目录",
        hint: "扫描 PS5 本地路径，例如 /data/homebrew 或外置存储目录。",
      };
    }
    return {
      icon: "SMB",
      title: "SMB 网络连接",
      hint: "共享名只填写 NAS 的共享层级，共享内目录通过浏览选择。",
    };
  }

  function setEditorProfileState(state, label) {
    ui.editorProfile.className = `editor-profile ${state || ""}`.trim();
    setText(ui.editorProfileState, label);
  }

  function updateEditorProfile() {
    const type = ui.editorType.value;
    const info = editorTypeInfo(type);
    setText(ui.editorProfileIcon, info.icon);
    setText(ui.editorProfileTitle, info.title);
    setText(ui.editorProfileHint, info.hint);
    const state = app.editor ? app.editor.testState : "idle";
    if (state === "testing") setEditorProfileState("testing", "正在测试");
    else if (state === "success") setEditorProfileState("success", "连接成功");
    else if (state === "error") setEditorProfileState("error", "测试失败");
    else if (state === "dirty") setEditorProfileState("dirty", "配置已修改");
    else setEditorProfileState("", "尚未测试");
  }

  function setEditorStep(node, state) {
    if (!node) return;
    node.classList.remove("active", "done", "error");
    if (state) node.classList.add(state);
  }

  function updateEditorSteps() {
    const type = ui.editorType.value;
    const hostReady = type === "local" || !!ui.editorHost.value.trim();
    const shareReady = type !== "smb" || !!parseShareAddress(
      ui.editorShare.value, ui.editorHost.value).share;
    const rootsReady = !!(app.editor && app.editor.roots.length);
    setEditorStep(ui.editorStepServer, hostReady ? "done" : "active");
    setEditorStep(ui.editorStepShare, shareReady ? "done" : hostReady ? "active" : "");
    setEditorStep(ui.editorStepRoots, rootsReady ? "done" : shareReady ? "active" : "");
  }

  function smbPreviewRow(label, value, ready) {
    const row = element("div", `smb-preview-row ${ready ? "ready" : ""}`.trim());
    row.appendChild(element("span", "", label));
    row.appendChild(element("code", "", value || "未填写"));
    row.appendChild(element("i"));
    return row;
  }

  function updateSmbPreview() {
    ui.smbPreview.replaceChildren();
    const type = ui.editorType.value;
    ui.smbPreview.hidden = type !== "smb";
    if (type !== "smb") return;
    const parsed = parseShareAddress(ui.editorShare.value, ui.editorHost.value);
    const port = ui.editorPort.value || "445";
    const root = app.editor && app.editor.roots.length ? app.editor.roots[0] : "/";
    ui.smbPreview.appendChild(smbPreviewRow("服务器",
      parsed.host ? `${parsed.host}:${port}` : "", !!parsed.host));
    ui.smbPreview.appendChild(smbPreviewRow("共享",
      parsed.share, !!parsed.share));
    ui.smbPreview.appendChild(smbPreviewRow("扫描入口", root, !!root));
    if (parsed.suffix !== "/") {
      ui.smbPreview.appendChild(element("p", "smb-preview-note",
        `已从共享输入中识别目录 ${parsed.suffix}，保存时会自动加入扫描目录。`));
    }
  }

  function setSmbGuideClasses(hostState, authState, shareState) {
    ui.smbGuideHost.className = hostState || "";
    ui.smbGuideAuth.className = authState || "";
    ui.smbGuideShare.className = shareState || "";
  }

  function renderSmbGuideIdle() {
    const parsed = parseShareAddress(ui.editorShare.value, ui.editorHost.value);
    const ready = !!parsed.host && !!parsed.share;
    setSmbGuideClasses(parsed.host ? "ready" : "", "", parsed.share ? "ready" : "");
    setText(ui.smbGuideStatus, ready ? "配置就绪" : "待补充");
    setText(ui.smbGuideHint, !parsed.host
      ? "先填写 NAS 主机或 IP 地址。"
      : !parsed.share
        ? "填写或枚举共享名；共享内的目录请在下方浏览选择。"
        : "点击“测试连接”读取第一个扫描目录，失败时会标出具体阶段。");
  }

  function renderSmbGuideResult(kind, message, failureIndex) {
    if (kind === "testing") {
      setSmbGuideClasses("active", "", "");
      setText(ui.smbGuideStatus, "正在测试");
    } else if (kind === "success") {
      setSmbGuideClasses("passed", "passed", "passed");
      setText(ui.smbGuideStatus, "连接成功");
    } else {
      const states = ["", "", ""];
      if (failureIndex === 0) states[0] = "error";
      if (failureIndex === 1) {
        states[0] = "passed";
        states[1] = "error";
      }
      if (failureIndex >= 2) {
        states[0] = "passed";
        states[1] = "passed";
        states[2] = "error";
      }
      setSmbGuideClasses(states[0], states[1], states[2]);
      setText(ui.smbGuideStatus, "测试失败");
    }
    setText(ui.smbGuideHint, message ||
      (kind === "success" ? "服务器、共享和扫描目录均可访问。" : ""));
  }

  function testFailureStage(type, message) {
    const text = String(message || "");
    if (type === "smb") {
      if (/目录|路径|PATH|NOT_FOUND|OBJECT_NAME|OBJECT_PATH|列表/i.test(text)) return 3;
      if (/用户名|密码错误|登录失败|认证失败|凭据无效|LOGON|AUTH|CREDENTIAL/i.test(text)) return 1;
      if (/共享名|共享.*权限|BAD_NETWORK_NAME|NETWORK_ACCESS_DENIED|ACCESS_DENIED|INVALID_PARAMETER/i.test(text)) return 2;
      return 0;
    }
    if (type === "ftp") {
      if (/登录|密码|账号|认证|LOGIN|530/i.test(text)) return 1;
      if (/目录|路径|读取|LIST|CWD|550/i.test(text)) return 2;
      return 0;
    }
    return 0;
  }

  function testFailureAdvice(type, stage) {
    if (type === "smb") {
      if (stage === 1) return "认证失败，请检查用户名、密码和域设置。";
      if (stage === 2) return "服务器已响应，但共享名不存在或当前账号没有访问权限。";
      if (stage === 3) return "共享可以访问，但扫描目录不存在或没有读取权限。";
      return "无法连接服务器，请检查 IP、端口、SMB 服务和 NAS 防火墙。";
    }
    if (type === "ftp") {
      if (stage === 1) return "FTP 登录失败，请检查用户名和密码。";
      if (stage === 2) return "FTP 已连接，但扫描目录不存在或没有读取权限。";
      return "无法连接 FTP 服务器，请检查地址、端口和防火墙。";
    }
    return "无法读取该本地目录，请检查路径和 PS5 文件权限。";
  }

  function renderEditorTestFlow(kind, message, failureIndex) {
    const type = ui.editorType.value;
    const stages = type === "smb"
      ? ["服务器与端口", "账号认证", "共享访问", "读取扫描目录"]
      : type === "ftp"
        ? ["服务器与端口", "FTP 登录", "读取扫描目录"]
        : ["读取本地目录"];
    ui.editorTestFlow.hidden = false;
    ui.editorTestSteps.replaceChildren();
    ui.editorTestSummary.className = kind;
    if (kind === "testing") {
      setText(ui.editorTestSummary, "正在连接远端...");
    } else if (kind === "success") {
      setText(ui.editorTestSummary, "全部检查通过");
    } else {
      setText(ui.editorTestSummary, "已定位失败阶段");
    }
    const failedAt = kind === "error" ? failureIndex : -1;
    stages.forEach((label, index) => {
      let state = "pending";
      if (kind === "testing" && index === 0) state = "active";
      if (kind === "success" || (kind === "error" && index < failedAt)) state = "done";
      if (kind === "error" && index === failedAt) state = "error";
      const step = element("div", `editor-test-step ${state}`);
      step.appendChild(element("i"));
      step.appendChild(element("span", "", label));
      if (kind === "error" && index === failedAt) {
        step.appendChild(element("small", "", message || "测试失败"));
      }
      ui.editorTestSteps.appendChild(step);
    });
  }

  function markEditorDirty() {
    if (!app.editor) return;
    updateEditorSteps();
    updateSmbPreview();
    if (app.editor.testState === "testing") return;
    if (app.editor.testState !== "dirty") {
      app.editor.testState = "dirty";
      ui.editorTestFlow.hidden = true;
      if (ui.editorType.value === "smb") renderSmbGuideIdle();
    }
    updateEditorProfile();
  }

  function openEditor(source, isNew) {
    rememberFocus("editor");
    app.editor = {
      source: clone(source),
      isNew: !!isNew,
      roots: Array.isArray(source.roots) && source.roots.length ? source.roots.slice() : ["/"],
      testState: "idle",
    };
    const current = app.editor.source;
    setText(ui.editorTitle, isNew ? "添加连接" : "编辑连接");
    ui.editorName.value = current.name || "";
    ui.editorType.value = current.type || "smb";
    ui.editorEnabled.checked = current.enabled !== false;
    ui.editorHost.value = current.host || "";
    ui.editorPort.value = current.port || defaultPort(current.type);
    ui.editorShare.value = current.share || "";
    ui.editorUser.value = current.username || "";
    ui.editorPassword.value = "";
    ui.editorDomain.value = current.domain || "";
    ui.editorLocalRoot.value = current.localRoot || "/data";
    setText(ui.passwordHint, current.has_password ? "已有保存的密码；留空保持不变。" : "留空表示无密码。");
    app.shares = [];
    ui.editorTest.disabled = false;
    setText(ui.editorTest, "测试连接");
    setText(ui.enumerateSharesButton, "枚举共享");
    ui.sharePicker.hidden = true;
    ui.shareResults.replaceChildren();
    ui.editorError.hidden = true;
    setText(ui.editorError, "");
    ui.editorTestFlow.hidden = true;
    ui.editorTestSteps.replaceChildren();
    updateEditorProfile();
    updateEditorType(false);
    renderRoots();
    setTransferOpen(false);
    ui.editorPanel.hidden = false;
    invalidateFocusCache();
    window.setTimeout(() => ui.editorName.focus({ preventScroll: true }), 20);
  }

  function closeEditor() {
    ui.editorPanel.hidden = true;
    ui.sharePicker.hidden = true;
    app.editor = null;
    app.shares = [];
    invalidateFocusCache();
    window.setTimeout(renderJob, 0);
    restoreFocus("editor", ui.connectionsList.querySelector("button, input") || ui.settingsClose);
  }

  function updateEditorType(changePort) {
    const type = ui.editorType.value;
    ui.networkFields.hidden = type === "local";
    ui.shareField.hidden = type !== "smb";
    ui.smbNote.hidden = type !== "smb";
    ui.domainField.hidden = type !== "smb";
    ui.localRootField.hidden = type !== "local";
    ui.enumerateSharesButton.disabled = type !== "smb";
    setText(ui.editorTestHint, type === "smb"
      ? "测试会连接主机并读取第一个扫描目录，不会自动保存。"
      : "测试使用当前草稿，不会自动保存。");
    if (changePort) {
      ui.editorPort.value = defaultPort(type) || "";
    }
    const typeName = type === "smb" ? "SMB 连接" : type === "ftp" ? "FTP 连接" : "本地游戏目录";
    if (!ui.editorName.value || ["SMB 连接", "FTP 连接", "本地游戏目录"].includes(ui.editorName.value)) {
      ui.editorName.value = typeName;
    }
    updateEditorProfile();
    updateEditorSteps();
    updateSmbPreview();
    if (type === "smb") renderSmbGuideIdle();
  }

  function renderRoots() {
    ui.rootsList.replaceChildren();
    if (!app.editor) return;
    if (!app.editor.roots.length) app.editor.roots = ["/"];
    setText(ui.editorRootsCount, `${app.editor.roots.length} 个目录`);
    for (const root of app.editor.roots) {
      const chip = element("div", "root-chip");
      chip.appendChild(element("span", "root-chip-icon", "/"));
      chip.appendChild(element("code", "", root));
      const remove = element("button", "remove-root", "×");
      remove.type = "button";
      remove.setAttribute("aria-label", `移除目录 ${root}`);
      remove.addEventListener("click", () => {
        app.editor.roots = app.editor.roots.filter((item) => item !== root);
        if (!app.editor.roots.length) app.editor.roots = ["/"];
        renderRoots();
        markEditorDirty();
      });
      chip.appendChild(remove);
      ui.rootsList.appendChild(chip);
    }
    updateSmbPreview();
  }

  function editorError(message) {
    editorStatus(message, "error");
  }

  function editorStatus(message, type = "info") {
    setText(ui.editorError, message);
    ui.editorError.className = `connection-status ${type}`.trim();
    ui.editorError.hidden = !message;
    ui.editorError.scrollIntoView({ block: "nearest" });
  }

  function readEditorSource(requireShare = true) {
    if (!app.editor || !app.settings) return null;
    const type = ui.editorType.value;
    const source = clone(app.editor.source);
    const originalShare = source.share;
    source.type = type;
    source.name = ui.editorName.value.trim();
    source.enabled = ui.editorEnabled.checked;
    source.host = ui.editorHost.value.trim();
    source.port = Number(ui.editorPort.value) || defaultPort(type);
    source.username = ui.editorUser.value.trim();
    source.domain = ui.editorDomain.value.trim();
    source.localRoot = ui.editorLocalRoot.value.trim() || "/";
    source.roots = app.editor.roots.map(normalizePath);

    if (type === "smb") {
      const parsed = parseShareAddress(ui.editorShare.value, source.host);
      source.host = parsed.host;
      source.share = parsed.share;
      ui.editorHost.value = source.host;
      ui.editorShare.value = source.share;
      if (originalShare && parsed.share !== originalShare && parsed.suffix === "/") {
        source.roots = ["/"];
      }
      if (parsed.suffix !== "/") {
        if (source.roots.length === 1 && source.roots[0] === "/") {
          source.roots[0] = parsed.suffix;
        } else if (!source.roots.includes(parsed.suffix)) {
          source.roots.push(parsed.suffix);
        }
      }
    } else {
      source.share = "";
    }
    source.roots = Array.from(new Set(source.roots));
    if (!source.roots.length) source.roots = ["/"];
    app.editor.roots = source.roots.slice();
    renderRoots();

    if (ui.editorPassword.value) {
      source.password = ui.editorPassword.value;
      source.has_password = true;
    } else {
      source.password = "";
    }

    if (!source.name) {
      editorError("请填写显示名称。");
      return null;
    }
    if (type !== "local" && !source.host) {
      editorError("请填写 NAS 的主机名或 IP 地址。");
      return null;
    }
    if (type === "smb" && requireShare && !source.share) {
      editorError("SMB 连接需要填写共享名，例如 NasBackups；共享内的子目录请用“浏览并添加”。");
      return null;
    }
    if (type === "local" && !source.localRoot) {
      editorError("本地来源需要填写根目录。");
      return null;
    }
    if (type === "local") {
      source.host = "";
      source.share = "";
      source.username = "";
      source.domain = "";
      source.password = "";
    } else if (type === "ftp") {
      source.share = "";
      source.domain = "";
    }

    return source;
  }

  function commitEditor() {
    const source = readEditorSource();
    if (!source) return null;
    const list = app.settings.sources;
    const index = list.findIndex((item) => item.id === source.id);
    if (index >= 0) list[index] = clone(source);
    else list.push(clone(source));
    renderConnections();
    ui.settingsHint.textContent = "有尚未保存的连接修改。";
    ui.editorError.hidden = true;
    updateEditorSteps();
    updateSmbPreview();
    updateEditorProfile();
    if (source.type === "smb") renderSmbGuideIdle();
    return source;
  }

  function collectGeneralSettings() {
    if (!app.settings) return;
    const port = Number(ui.settingPort.value);
    const interval = Number(ui.settingInterval.value);
    const depth = Number(ui.settingDepth.value);
    if (port >= 1024 && port <= 65535) app.settings.port = port;
    if (interval >= 30 && interval <= 86400) app.settings.scan_interval_seconds = interval;
    if (depth >= 0 && depth <= 12) app.settings.max_scan_depth = depth;
    app.settings.auto_scan = ui.settingAutoScan.checked;
    app.settings.open_browser_on_start = ui.settingOpenBrowser.checked;
    app.settings.keep_awake = ui.settingKeepAwake.checked;
  }

  async function persistSettings(showSuccess) {
    collectGeneralSettings();
    const body = clone(app.settings);
    for (const source of body.sources) {
      if (!source.password) delete source.password;
      delete source.has_password;
    }
    await api("/api/settings", { method: "PUT", body });
    app.settings = clone(body);
    if (showSuccess) showToast("设置已保存，扫描任务已开始", "success");
  }

  async function saveSettingsAndClose() {
    if (!app.settings) return;
    ui.settingsSave.disabled = true;
    try {
      const oldPort = app.state && app.state.server ? app.state.server.port : null;
      await persistSettings(false);
      const newPort = app.settings.port;
      closeSettings();
      showToast("设置已保存，正在重新扫描", "success");
      if (oldPort && newPort && oldPort !== newPort) {
        showToast(`端口已改为 ${newPort}，需要重启 payload 后生效`, "error");
      }
      app.librarySignature = "";
      app.headerKey = "";
      app.groupsRevision = -1;
      app.detailKey = "";
      scheduleStatusPoll(500);
    } catch (error) {
      showToast(`保存失败：${error.message}`, "error");
      setText(ui.settingsHint, `保存失败：${error.message}`);
    } finally {
      ui.settingsSave.disabled = false;
    }
  }

  async function testSource(sourceId) {
    const source = app.settings && app.settings.sources.find((item) => item.id === sourceId);
    if (!source) return;
    try {
      const result = await api("/api/test", {
        method: "POST",
        body: sourceRequestPayload(source),
      });
      showToast(result.message || "连接成功", "success");
    } catch (error) {
      showToast(`连接测试失败：${error.message}`, "error");
    }
  }

  async function testEditorSource() {
    const source = readEditorSource();
    if (!source) return;
    const editor = app.editor;
    editor.testState = "testing";
    updateEditorProfile();
    renderEditorTestFlow("testing");
    if (ui.editorType.value === "smb") renderSmbGuideResult("testing");
    ui.editorTest.disabled = true;
    setText(ui.editorTest, "正在测试...");
    try {
      const result = await api("/api/test", {
        method: "POST",
        body: sourceRequestPayload(source),
      });
      if (app.editor !== editor) return;
      editor.testState = "success";
      updateEditorProfile();
      renderEditorTestFlow("success");
      if (ui.editorType.value === "smb") renderSmbGuideResult("success");
      editorStatus(`${result.message || "连接成功"}。测试使用当前草稿，尚未保存。`, "success");
      showToast(result.message || "连接成功", "success");
    } catch (error) {
      if (app.editor !== editor) return;
      const failureIndex = testFailureStage(ui.editorType.value, error.message);
      const advice = testFailureAdvice(ui.editorType.value, failureIndex);
      editor.testState = "error";
      updateEditorProfile();
      renderEditorTestFlow("error", error.message, failureIndex);
      if (ui.editorType.value === "smb") {
        renderSmbGuideResult("error", advice, failureIndex);
      }
      editorError(`连接失败：${advice}`);
    } finally {
      if (app.editor === editor) {
        ui.editorTest.disabled = false;
        setText(ui.editorTest, "测试连接");
      }
    }
  }

  async function browseEditorSource() {
    const source = readEditorSource();
    if (!source) return;
    ui.browseRootButton.disabled = true;
    try {
      app.browser.source = clone(source);
      setTransferOpen(false);
      rememberFocus("browser");
      ui.browserPanel.hidden = false;
      invalidateFocusCache();
      ui.browserSourceLabel.textContent = `${sourceTypeLabel(source.type)} · ${sourceEndpoint(source)}`;
      setText(ui.browserHint, "正在读取目录...");
      renderBrowserBreadcrumbs(source.roots[0] || "/");
      await loadBrowser(source.roots[0] || "/");
    } catch (error) {
      editorError(`无法打开目录浏览：${error.message}`);
    } finally {
      ui.browseRootButton.disabled = false;
    }
  }

  async function loadBrowser(path) {
    if (!app.browser.source) return;
    const token = ++app.browser.token;
    const nextPath = normalizePath(path);
    renderBrowserBreadcrumbs(nextPath);
    syncBrowserChooseState(nextPath);
    const loading = element("div", "browser-empty");
    loading.appendChild(element("strong", "", "正在读取目录"));
    loading.appendChild(document.createTextNode("正在连接远端并读取文件列表..."));
    ui.browserList.replaceChildren(loading);
    const status = ui.browserHint.closest(".browser-status");
    if (status) status.classList.remove("error");
    setText(ui.browserHint, "正在读取目录...");
    try {
      const body = sourceRequestPayload(app.browser.source);
      body.path = nextPath;
      const result = await api("/api/browse", { method: "POST", body });
      if (token !== app.browser.token) return;
      app.browser.path = result.path || nextPath;
      ui.browserPath.value = app.browser.path;
      renderBrowserBreadcrumbs(app.browser.path);
      syncBrowserChooseState(app.browser.path);
      ui.browserList.replaceChildren();
      const entries = Array.isArray(result.entries) ? result.entries : [];
      if (!entries.length) {
        const empty = element("div", "browser-empty");
        empty.appendChild(element("strong", "", "这个目录是空的"));
        empty.appendChild(document.createTextNode("返回上一级，或把当前目录直接加入扫描范围。"));
        ui.browserList.appendChild(empty);
      }
      for (const entry of entries) {
        const isDir = !!entry.isDir;
        const row = element(isDir ? "button" : "div", `browser-entry ${isDir ? "folder" : "file"}`.trim());
        if (isDir) row.type = "button";
        const main = element("span", "browser-entry-main");
        main.appendChild(element("span", `browser-entry-icon ${isDir ? "folder" : "file"}`));
        main.appendChild(element("span", "browser-entry-name", entry.name));
        row.appendChild(main);
        row.appendChild(element("span", "browser-entry-meta", isDir ? "文件夹" : "文件"));
        row.appendChild(element("span", "browser-entry-meta", isDir ? "—" : formatBytes(entry.size)));
        row.appendChild(element("span", "browser-entry-meta", formatFileTime(entry.mtime)));
        if (isDir) {
          row.addEventListener("click", () => {
            const child = app.browser.path === "/" ? `/${entry.name}` : `${app.browser.path}/${entry.name}`;
            loadBrowser(child);
          });
        }
        ui.browserList.appendChild(row);
      }
      const directoryCount = entries.filter((entry) => entry.isDir).length;
      setText(ui.browserHint, `${app.browser.path} · ${directoryCount} 个子目录 · ${entries.length} 个项目`);
      window.setTimeout(() => ui.browserList.querySelector("button")?.focus({ preventScroll: true }), 20);
    } catch (error) {
      if (token !== app.browser.token) return;
      const failed = element("div", "browser-empty error");
      failed.appendChild(element("strong", "", "无法读取此目录"));
      failed.appendChild(document.createTextNode(error.message));
      ui.browserList.replaceChildren(failed);
      if (status) status.classList.add("error");
      setText(ui.browserHint, `读取失败：${error.message}`);
    }
  }

  function closeBrowser() {
    ui.browserPanel.hidden = true;
    app.browser.source = null;
    const status = ui.browserHint.closest(".browser-status");
    if (status) status.classList.remove("error");
    setText(ui.browserHint, "选择目录后可将其加入扫描范围。");
    invalidateFocusCache();
    window.setTimeout(renderJob, 0);
    restoreFocus("browser", ui.browseRootButton);
  }

  function renderBrowserBreadcrumbs(path) {
    const normalized = normalizePath(path);
    ui.browserBreadcrumbs.replaceChildren();
    const root = element("button",
      normalized === "/" ? "browser-crumb active" : "browser-crumb",
      "根目录");
    root.type = "button";
    root.dataset.path = "/";
    root.addEventListener("click", () => loadBrowser("/"));
    ui.browserBreadcrumbs.appendChild(root);

    let current = "";
    const parts = normalized.split("/").filter(Boolean);
    parts.forEach((part, index) => {
      current += `/${part}`;
      ui.browserBreadcrumbs.appendChild(element("span", "browser-crumb-separator", "/"));
      const isLast = index === parts.length - 1;
      if (isLast) {
        const active = element("span", "browser-crumb active", part);
        active.setAttribute("aria-current", "page");
        ui.browserBreadcrumbs.appendChild(active);
      } else {
        const button = element("button", "browser-crumb", part);
        button.type = "button";
        button.dataset.path = current;
        button.addEventListener("click", () => loadBrowser(current));
        ui.browserBreadcrumbs.appendChild(button);
      }
    });
    ui.browserUp.disabled = normalized === "/";
  }

  function syncBrowserChooseState(path) {
    const normalized = normalizePath(path);
    const included = !!(app.editor && app.editor.roots.includes(normalized));
    ui.browserRootStatus.className = included
      ? "browser-root-status included"
      : "browser-root-status";
    setText(ui.browserRootStatus, included ? "已在扫描范围" : "当前目录尚未加入");
    ui.browserChoose.disabled = included;
    setText(ui.browserChoose, included ? "已在扫描范围" : "添加当前目录");
  }

  function chooseBrowserPath() {
    if (!app.editor) return;
    const path = normalizePath(app.browser.path);
    if (!app.editor.roots.includes(path)) app.editor.roots.push(path);
    renderRoots();
    markEditorDirty();
    closeBrowser();
    showToast(`已加入扫描目录：${path}`, "success");
  }

  function renderShares(shares) {
    ui.shareResults.replaceChildren();
    app.shares = Array.isArray(shares) ? shares : [];
    if (!app.shares.length) {
      const empty = element("div", "browser-empty");
      empty.appendChild(element("strong", "", "没有发现磁盘共享"));
      empty.appendChild(document.createTextNode("NAS 可能禁用了 IPC$ 共享枚举，请直接手动填写共享名。"));
      ui.shareResults.appendChild(empty);
      return;
    }
    for (const share of app.shares) {
      const button = element("button", "share-result");
      button.type = "button";
      const copy = element("span", "share-result-copy");
      copy.appendChild(element("strong", "", share.name));
      copy.appendChild(element("small", "", share.remark || "SMB 磁盘共享"));
      button.appendChild(copy);
      button.appendChild(element("span", "", "选择"));
      button.addEventListener("click", async () => {
        app.editor.roots = ["/"];
        renderRoots();
        ui.editorShare.value = share.name;
        markEditorDirty();
        editorStatus(`已选择共享“${share.name}”，正在打开共享根目录。`, "success");
        ui.sharePicker.hidden = true;
        await browseEditorSource();
      });
      ui.shareResults.appendChild(button);
    }
  }

  async function enumerateShares() {
    const source = readEditorSource(false);
    if (!source) return;
    const editor = app.editor;
    ui.enumerateSharesButton.disabled = true;
    setText(ui.enumerateSharesButton, "正在枚举...");
    ui.sharePicker.hidden = false;
    ui.shareResults.replaceChildren(element("div", "browser-empty", "正在读取 NAS 共享列表..."));
    window.setTimeout(() => ui.sharePicker.scrollIntoView({ block: "nearest" }), 20);
    try {
      const result = await api("/api/shares", {
        method: "POST",
        body: sourceRequestPayload(source),
      });
      if (app.editor !== editor || ui.sharePicker.hidden) return;
      const shares = Array.isArray(result.shares) ? result.shares : [];
      renderShares(shares);
      editorStatus(`已从 ${result.host || source.host} 获取 ${shares.length} 个磁盘共享。`, "success");
      if (shares.length === 1 && !ui.editorShare.value) {
        ui.editorShare.value = shares[0].name;
      }
    } catch (error) {
      if (app.editor !== editor || ui.sharePicker.hidden) return;
      ui.shareResults.replaceChildren();
      const failed = element("div", "browser-empty error");
      failed.appendChild(element("strong", "", "共享枚举失败"));
      failed.appendChild(document.createTextNode(error.message));
      ui.shareResults.appendChild(failed);
      editorError(`无法枚举共享：${error.message}`);
    } finally {
      if (app.editor === editor) {
        ui.enumerateSharesButton.disabled = false;
        setText(ui.enumerateSharesButton, "枚举共享");
      }
    }
  }

  function stopLogPolling() {
    if (app.logTimer) window.clearInterval(app.logTimer);
    app.logTimer = 0;
  }

  function updateSettingsFooter(tab) {
    const editable = tab === "global" || tab === "connections";
    ui.settingsSave.hidden = !editable;
    setText(ui.settingsCancel, editable ? "取消" : "关闭");
    setText(ui.settingsHint,
      tab === "logs"
        ? "运行日志会自动刷新，仅用于查看诊断信息。"
        : tab === "overview"
          ? "选择左侧分类以管理连接、全局行为或运行日志。"
          : "修改后保存，游戏库会自动刷新。");
  }

  function switchSettingsTab(name, focusTab) {
    const valid = ["overview", "global", "connections", "logs"];
    const tab = valid.includes(name) ? name : "overview";
    app.settingsTab = tab;
    for (const button of ui.settingsNav.querySelectorAll("[data-settings-tab]")) {
      const active = button.dataset.settingsTab === tab;
      button.classList.toggle("is-active", active);
      button.setAttribute("aria-selected", active ? "true" : "false");
    }
    for (const page of ui.settingsPanel.querySelectorAll("[data-settings-page]")) {
      page.classList.toggle("is-active", page.dataset.settingsPage === tab);
    }
    updateSettingsFooter(tab);
    if (tab === "logs") {
      refreshLogs();
      stopLogPolling();
      app.logTimer = window.setInterval(() => {
        if (!ui.settingsPanel.hidden && app.settingsTab === "logs") refreshLogs();
      }, 2600);
    } else {
      stopLogPolling();
    }
    if (focusTab) {
      const button = ui.settingsNav.querySelector(`[data-settings-tab="${tab}"]`);
      if (button) button.focus({ preventScroll: true });
    }
  }

  async function refreshLogs() {
    if (app.logBusy) return;
    app.logBusy = true;
    try {
      const followTail = ui.logText.scrollHeight - ui.logText.scrollTop -
        ui.logText.clientHeight < 28;
      const data = await api("/api/log");
      setText(ui.logText, data.log || "暂无日志");
      if (followTail) ui.logText.scrollTop = ui.logText.scrollHeight;
    } catch (error) {
      setText(ui.logText, `读取日志失败：${error.message}`);
    } finally {
      app.logBusy = false;
    }
  }

  function topLayer() {
    if (!ui.launchScreen.hidden) return ui.launchScreen;
    if (!ui.confirmPanel.hidden) return ui.confirmPanel;
    if (!ui.browserPanel.hidden) return ui.browserPanel;
    if (!ui.editorPanel.hidden) return ui.editorPanel;
    if (!ui.settingsPanel.hidden) return ui.settingsPanel;
    if (!ui.detailPanel.hidden) return ui.detailPanel;
    if (!ui.transferPanel.hidden) return ui.transferPanel;
    return ui.shell;
  }

  function goBack() {
    if (!ui.confirmPanel.hidden) return closeConfirm();
    if (!ui.browserPanel.hidden) return closeBrowser();
    if (!ui.editorPanel.hidden) return closeEditor();
    if (!ui.settingsPanel.hidden) return closeSettings();
    if (!ui.detailPanel.hidden) return closeDetail();
    if (!ui.transferPanel.hidden) return dismissTransfer();
  }

  function buildFocusCache(scope) {
    const selector = "button:not([disabled]), input:not([disabled]), select:not([disabled]), [tabindex='0']";
    const nodes = [];
    const rects = new Map();
    for (const node of scope.querySelectorAll(selector)) {
      if (node.hidden || node.closest("[hidden]")) continue;
      const style = window.getComputedStyle(node);
      if (style.display === "none" || style.visibility === "hidden") continue;
      const rect = node.getBoundingClientRect();
      if (rect.width <= 0 || rect.height <= 0) continue;
      nodes.push(node);
      rects.set(node, rect);
    }
    return { scope, nodes, rects };
  }

  function focusCacheFor(scope) {
    if (!app.focusCache || app.focusCache.scope !== scope) {
      app.focusCache = buildFocusCache(scope);
    }
    return app.focusCache;
  }

  function focusableElements(scope) {
    return scope ? focusCacheFor(scope).nodes : [];
  }

  function navigate(direction) {
    const scope = topLayer();
    const cache = focusCacheFor(scope);
    const items = cache.nodes;
    if (!items.length) return;
    let current = document.activeElement;
    if (!current || !items.includes(current)) {
      items[0].focus({ preventScroll: false });
      return;
    }
    const origin = current.getBoundingClientRect();
    const ox = origin.left + origin.width / 2;
    const oy = origin.top + origin.height / 2;
    let best = null;
    let bestScore = Infinity;
    for (const candidate of items) {
      if (candidate === current) continue;
      const rect = cache.rects.get(candidate);
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;
      const dx = cx - ox;
      const dy = cy - oy;
      let primary = 0;
      let secondary = 0;
      if (direction === "left" && dx < -4) { primary = -dx; secondary = Math.abs(dy); }
      else if (direction === "right" && dx > 4) { primary = dx; secondary = Math.abs(dy); }
      else if (direction === "up" && dy < -4) { primary = -dy; secondary = Math.abs(dx); }
      else if (direction === "down" && dy > 4) { primary = dy; secondary = Math.abs(dx); }
      else continue;
      const score = primary + secondary * 0.58;
      if (score < bestScore) {
        bestScore = score;
        best = candidate;
      }
    }
    if (best) best.focus({ preventScroll: false });
  }

  function activateCurrent() {
    const current = document.activeElement;
    const scope = topLayer();
    if (!current || !scope.contains(current)) {
      const items = focusableElements(scope);
      if (items[0]) items[0].focus();
      return;
    }
    if (current.matches("button, [role='button']")) {
      current.click();
    } else if (current.matches("input[type='checkbox']")) {
      current.click();
    } else if (current.matches("input, select")) {
      current.focus();
    }
  }

  function isEditable(node) {
    return !!node && (node.matches("input, textarea, select") || node.isContentEditable);
  }

  document.addEventListener("keydown", (event) => {
    if (event.defaultPrevented) return;
    if (!ui.formatMenu.hidden && event.key === "Escape") {
      event.preventDefault();
      setFormatMenu(false);
      ui.formatFilterButton.focus({ preventScroll: true });
      return;
    }
    if (event.key === "Escape" || (event.key === "Backspace" && !isEditable(event.target))) {
      if (topLayer() !== ui.shell) {
        event.preventDefault();
        goBack();
      }
      return;
    }
    const directions = {
      ArrowLeft: "left",
      ArrowRight: "right",
      ArrowUp: "up",
      ArrowDown: "down",
    };
    const direction = directions[event.key];
    if (!direction) return;
    if (isEditable(event.target)) return;
    event.preventDefault();
    navigate(direction);
  });

  window.addEventListener("scroll", invalidateFocusCache, true);
  window.addEventListener("resize", invalidateFocusCache);
  if (typeof MutationObserver === "function") {
    const focusObserver = new MutationObserver((mutations) => {
      if (!app.focusCache) return;
      for (const mutation of mutations) {
        const node = mutation.target.nodeType === 1
          ? mutation.target
          : mutation.target.parentElement;
        if (node && node.closest(
          "#transferShell, #toastStack, #scanStatus, #sourcePills, #noticeStrip, #logText")) {
          continue;
        }
        invalidateFocusCache();
        return;
      }
    });
    focusObserver.observe(document.body, {
      subtree: true,
      childList: true,
      attributes: true,
      attributeFilter: ["hidden", "class", "disabled", "style"],
    });
  }

  ui.filters.addEventListener("click", (event) => {
    const button = event.target.closest("[data-filter]");
    if (!button) return;
    setFormatMenu(false);
    app.filter = button.dataset.filter;
    for (const filter of ui.filters.querySelectorAll("[data-filter]")) {
      filter.classList.toggle("is-active", filter === button);
    }
    renderLibrary(true);
  });

  ui.formatFilterButton.addEventListener("click", () => {
    setFormatMenu(ui.formatMenu.hidden);
  });

  ui.formatMenu.addEventListener("click", (event) => {
    const option = event.target.closest("[data-format]");
    if (!option) return;
    event.preventDefault();
    event.stopPropagation();
    setFormat(option.dataset.format, { focusButton: true });
  });

  ui.formatMenu.addEventListener("keydown", (event) => {
    const options = Array.from(ui.formatMenu.querySelectorAll("[data-format]"));
    const index = options.indexOf(document.activeElement);
    if (index < 0) return;
    let next = index;
    if (event.key === "ArrowDown") next = (index + 1) % options.length;
    else if (event.key === "ArrowUp") next = (index - 1 + options.length) % options.length;
    else if (event.key === "Home") next = 0;
    else if (event.key === "End") next = options.length - 1;
    else return;
    event.preventDefault();
    event.stopPropagation();
    options[next].focus({ preventScroll: true });
  });

  ui.formatFilter.addEventListener("focusout", () => {
    window.setTimeout(() => {
      if (!ui.formatFilter.contains(document.activeElement)) {
        setFormatMenu(false);
      }
    }, 0);
  });

  document.addEventListener("pointerdown", (event) => {
    if (!ui.formatMenu.hidden && !ui.formatFilter.contains(event.target)) {
      setFormatMenu(false);
    }
  });

  ui.search.addEventListener("input", () => {
    app.query = ui.search.value;
    renderLibrary(true);
  });
  ui.library.addEventListener("focusin", (event) => {
    const card = event.target.closest && event.target.closest("[data-game-id]");
    if (card) preloadDetailArt(findGameGroup(card.dataset.gameId));
  });
  ui.library.addEventListener("pointerover", (event) => {
    const card = event.target.closest && event.target.closest("[data-game-id]");
    if (card) preloadDetailArt(findGameGroup(card.dataset.gameId));
  });
  ui.scanButton.addEventListener("click", requestScan);
  ui.settingsButton.addEventListener("click", openSettings);
  ui.transferButton.addEventListener("click", () => {
    if (ui.transferPanel.hidden) {
      setTransferOpen(true);
    } else {
      dismissTransfer();
    }
  });
  ui.transferClose.addEventListener("click", dismissTransfer);
  ui.cancelJobButton.addEventListener("click", cancelInstallJob);
  ui.clearHistoryButton.addEventListener("click", clearTransferHistory);
  ui.detailClose.addEventListener("click", closeDetail);
  ui.detailBackdrop.addEventListener("click", closeDetail);
  ui.launchButton.addEventListener("click", launchGame);
  ui.installButton.addEventListener("click", () => {
    const group = findGameGroup(app.selectedGameId);
    if (!group) return;
    const game = group.primary;
    if (game.type === "folder" && !game.libraryCopy && gameSizeInfo(group, game).status === "loading") {
      showToast("正在统计游戏大小，请稍候");
      return;
    }
    beginInstall(game, game.type === "pkg"
      ? "system"
      : game.packing !== "plain" ? "homebrew" : "library");
  });
  ui.detailPackageList.addEventListener("click", (event) => {
    const button = event.target.closest("[data-package-id]");
    if (!button || button.disabled) return;
    const game = findGame(button.dataset.packageId);
    if (game) beginInstall(game, "system");
  });
  ui.settingsNav.addEventListener("click", (event) => {
    const button = event.target.closest("[data-settings-tab]");
    if (button) switchSettingsTab(button.dataset.settingsTab, false);
  });
  ui.settingsClose.addEventListener("click", closeSettings);
  ui.settingsCancel.addEventListener("click", closeSettings);
  ui.settingsSave.addEventListener("click", saveSettingsAndClose);
  ui.addSmbButton.addEventListener("click", () => openEditor(newSource("smb"), true));
  ui.addFtpButton.addEventListener("click", () => openEditor(newSource("ftp"), true));
  ui.addLocalButton.addEventListener("click", () => openEditor(newSource("local"), true));

  ui.connectionsList.addEventListener("click", async (event) => {
    const button = event.target.closest("[data-action]");
    if (!button || button.tagName === "INPUT") return;
    const id = button.dataset.id;
    const source = app.settings && app.settings.sources.find((item) => item.id === id);
    if (!source) return;
    if (button.dataset.action === "edit") {
      openEditor(source, false);
    } else if (button.dataset.action === "test") {
      button.disabled = true;
      setText(button, "测试中");
      try {
        await testSource(id);
      } finally {
        button.disabled = false;
        setText(button, "测试连接");
      }
    } else if (button.dataset.action === "delete") {
      askConfirm("删除连接", `删除“${source.name || source.id}”？游戏文件不会被删除。`, "删除", async () => {
        app.settings.sources = app.settings.sources.filter((item) => item.id !== id);
        renderConnections();
        ui.settingsHint.textContent = "连接已从配置中移除，点击保存后生效。";
      });
    }
  });

  ui.connectionsList.addEventListener("change", (event) => {
    const toggle = event.target.closest("[data-action='toggle']");
    if (!toggle || !app.settings) return;
    const source = app.settings.sources.find((item) => item.id === toggle.dataset.id);
    if (!source) return;
    source.enabled = toggle.checked;
    renderConnections();
    ui.settingsHint.textContent = "启用状态已修改，点击保存后生效。";
  });

  ui.editorType.addEventListener("change", () => {
    updateEditorType(true);
    markEditorDirty();
  });
  for (const field of [
    ui.editorName, ui.editorHost, ui.editorPort, ui.editorShare,
    ui.editorUser, ui.editorPassword, ui.editorDomain, ui.editorLocalRoot,
  ]) {
    field.addEventListener("input", markEditorDirty);
  }
  ui.editorEnabled.addEventListener("change", markEditorDirty);
  ui.editorClose.addEventListener("click", closeEditor);
  ui.editorCancel.addEventListener("click", closeEditor);
  ui.editorApply.addEventListener("click", () => {
    if (!commitEditor()) return;
    closeEditor();
  });
  ui.editorTest.addEventListener("click", testEditorSource);
  ui.enumerateSharesButton.addEventListener("click", enumerateShares);
  ui.closeSharePicker.addEventListener("click", () => {
    ui.sharePicker.hidden = true;
    window.setTimeout(() => ui.enumerateSharesButton.focus({ preventScroll: true }), 20);
  });
  ui.browseRootButton.addEventListener("click", browseEditorSource);
  ui.browserClose.addEventListener("click", closeBrowser);
  ui.browserCancel.addEventListener("click", closeBrowser);
  ui.browserChoose.addEventListener("click", chooseBrowserPath);
  ui.browserUp.addEventListener("click", () => loadBrowser(parentPath(app.browser.path)));
  ui.browserGo.addEventListener("click", () => loadBrowser(ui.browserPath.value));
  ui.browserRefresh.addEventListener("click", () => loadBrowser(app.browser.path));
  ui.browserPath.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      loadBrowser(ui.browserPath.value);
    }
  });
  ui.logRefresh.addEventListener("click", refreshLogs);
  ui.confirmCancel.addEventListener("click", closeConfirm);
  ui.confirmOk.addEventListener("click", async () => {
    if (app.confirmBusy || !app.confirmCallback) return;
    app.confirmBusy = true;
    ui.confirmOk.disabled = true;
    try {
      await app.confirmCallback();
      app.confirmCallback = null;
      ui.confirmPanel.hidden = true;
      invalidateFocusCache();
      restoreFocus("confirm", null);
    } catch (error) {
      showToast(error.message || String(error), "error");
    } finally {
      app.confirmBusy = false;
      ui.confirmOk.disabled = false;
    }
  });

  ui.detailPanel.addEventListener("click", (event) => {
    if (event.target === ui.detailPanel) closeDetail();
  });
  ui.settingsPanel.addEventListener("click", (event) => {
    const jump = event.target.closest("[data-settings-jump]");
    if (jump) {
      switchSettingsTab(jump.dataset.settingsJump, true);
      return;
    }
    if (event.target === ui.settingsPanel) closeSettings();
  });
  ui.editorPanel.addEventListener("click", (event) => {
    if (event.target === ui.editorPanel) closeEditor();
  });
  ui.browserPanel.addEventListener("click", (event) => {
    if (event.target === ui.browserPanel) closeBrowser();
  });
  document.addEventListener("click", (event) => {
    if (!ui.transferPanel.hidden && !ui.transferShell.contains(event.target)) {
      dismissTransfer();
    }
  });

  function setupGamepad() {
    if (!navigator.getGamepads) return;
    const state = {
      up: { pressed: false, at: 0, repeatAt: 0 },
      down: { pressed: false, at: 0, repeatAt: 0 },
      left: { pressed: false, at: 0, repeatAt: 0 },
      right: { pressed: false, at: 0, repeatAt: 0 },
      a: { pressed: false, at: 0, repeatAt: 0 },
      b: { pressed: false, at: 0, repeatAt: 0 },
    };
    const repeatDelay = 350;
    const repeatInterval = 120;
    const trigger = (key, active, action, repeatable = true) => {
      const now = performance.now();
      const item = state[key];
      if (!active) {
        item.pressed = false;
        item.repeatAt = 0;
        return;
      }
      if (!item.pressed) {
        item.pressed = true;
        item.at = now;
        item.repeatAt = now + repeatDelay;
        action();
      } else if (repeatable && now >= item.repeatAt) {
        item.repeatAt = now + repeatInterval;
        action();
      }
    };
    function tick() {
      const pads = navigator.getGamepads();
      let pad = null;
      for (const candidate of pads) {
        if (candidate && candidate.connected) { pad = candidate; break; }
      }
      if (pad) {
        const button = (index) => !!(pad.buttons[index] && pad.buttons[index].pressed);
        const axisX = pad.axes[0] || 0;
        const axisY = pad.axes[1] || 0;
        trigger("up", button(12) || axisY < -0.62, () => navigate("up"));
        trigger("down", button(13) || axisY > 0.62, () => navigate("down"));
        trigger("left", button(14) || axisX < -0.62, () => navigate("left"));
        trigger("right", button(15) || axisX > 0.62, () => navigate("right"));
        trigger("a", button(0), activateCurrent, false);
        trigger("b", button(1), goBack, false);
      }
      window.requestAnimationFrame(tick);
    }
    window.requestAnimationFrame(tick);
  }

  async function boot() {
    setupGamepad();
    await refreshState(true);
    if (!app.state) {
      ui.library.replaceChildren(element("div", "empty-state", "正在连接 Game Center payload..."));
    }
    scheduleStatusPoll();
    document.addEventListener("visibilitychange", () => {
      if (!document.hidden) {
        if (app.launchScreenActive) {
          window.setTimeout(() => {
            if (app.launchScreenActive) finishLaunchScreen(false);
          }, 250);
        }
        refreshState(true).finally(() => scheduleStatusPoll());
      }
    });
  }

  boot();
})();
