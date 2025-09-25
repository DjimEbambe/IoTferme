import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';

const ensureTrailingSlash = (value) => (value && !value.endsWith('/') ? `${value}/` : value || '/');
const MODEL_ROOT = ensureTrailingSlash((typeof window !== 'undefined' && window.__TWIN__?.modelRoot) || '/twins/');

const normalizeKey = (value) => {
  if (value === undefined || value === null) return '';
  return value
    .toString()
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .toUpperCase()
    .replace(/[^A-Z0-9]/g, '');
};

const CANONICAL_LOOKUP = {
  BLDGA: 'BLDG:A',
  BATIMENT1: 'BLDG:A',
  BAT1: 'BLDG:A',
  BATIMENTA: 'BLDG:A',
  BLDGB: 'BLDG:B',
  BATIMENT2: 'BLDG:B',
  BAT2: 'BLDG:B',
  BATIMENTB: 'BLDG:B',
  SUPPROV: 'SUP:PROV',
  COUVEUSE: 'SUP:PROV',
};

const FRIENDLY_LABELS = {
  'BLDG:A': 'Bâtiment 1',
  'BLDG:B': 'Bâtiment 2',
  'SUP:PROV': 'Couveuse',
};

const MODEL_LIBRARY = {
  environment: { file: 'tout.glb', type: 'environment' },
  'BLDG:A': { file: 'batiment1.glb', type: 'asset' },
  'BLDG:B': { file: 'batiment2.glb', type: 'asset' },
  'SUP:PROV': { procedural: 'couveuseCube', type: 'asset' },
};

const HEATMAP_PRESETS = {
  temperature: { min: 15, max: 35, start: new THREE.Color(0.12, 0.22, 0.65), end: new THREE.Color(0.95, 0.45, 0.2) },
  humidity: { min: 45, max: 95, start: new THREE.Color(0.18, 0.42, 0.9), end: new THREE.Color(0.72, 0.92, 0.34) },
  air_quality: { min: 70, max: 100, start: new THREE.Color(0.25, 0.25, 0.55), end: new THREE.Color(0.18, 0.8, 0.4) },
};

const DEFAULT_COLORS = {
  'BLDG:A': new THREE.Color('#61a8ff'),
  'BLDG:B': new THREE.Color('#66d5c2'),
  'SUP:PROV': new THREE.Color('#f7ba56'),
  default: new THREE.Color('#6f9bff'),
};

const canonicalAssetId = (asset) => {
  if (!asset) return null;
  const candidates = [asset.asset_id, asset?.metadata?.display_name, asset?.label, asset?.name];
  for (const candidate of candidates) {
    const key = normalizeKey(candidate);
    const canonical = CANONICAL_LOOKUP[key];
    if (canonical && FRIENDLY_LABELS[canonical]) return canonical;
  }
  return null;
};

const loader = new GLTFLoader();
const modelCache = new Map();

const loadGLB = (file, scene) => new Promise((resolve, reject) => {
  const url = `${MODEL_ROOT}${file}`;
  loader.load(
    url,
    (gltf) => {
      const root = new THREE.Group();
      root.add(gltf.scene);
      gltf.scene.traverse((child) => {
        if (child.isMesh) {
          child.castShadow = true;
          child.receiveShadow = true;
        }
      });
      resolve(root);
    },
    undefined,
    (error) => reject(error)
  );
});

const getModel = async (key) => {
  const config = MODEL_LIBRARY[key];
  if (!config?.file) return null;
  if (!modelCache.has(key)) {
    const root = await loadGLB(config.file);
    modelCache.set(key, root);
  }
  const stored = modelCache.get(key);
  return stored.clone(true);
};

const createCouveuseCube = () => {
  const group = new THREE.Group();
  const body = new THREE.Mesh(
    new THREE.BoxGeometry(6, 3, 4.5),
    new THREE.MeshStandardMaterial({ color: 0x5fb2ff, transparent: true, opacity: 0.7, metalness: 0.1, roughness: 0.3 })
  );
  body.position.y = 1.5;
  const visor = new THREE.Mesh(
    new THREE.BoxGeometry(6.2, 0.2, 0.3),
    new THREE.MeshStandardMaterial({ color: 0xffb347, emissive: 0xff8c1a, metalness: 0.2, roughness: 0.2 })
  );
  visor.position.set(0, 2.9, -2.25);
  group.add(body, visor);
  return group;
};

const computeOverlay = (object, fallbackColor) => {
  const box = new THREE.Box3().setFromObject(object);
  if (!Number.isFinite(box.min.x) || !Number.isFinite(box.max.x)) return null;
  const size = new THREE.Vector3();
  const center = new THREE.Vector3();
  box.getSize(size);
  box.getCenter(center);
  const geometry = new THREE.BoxGeometry(Math.max(size.x, 1), Math.max(size.y, 1), Math.max(size.z, 1));
  const material = new THREE.MeshBasicMaterial({ color: fallbackColor.clone(), transparent: true, opacity: 0.25 });
  const overlay = new THREE.Mesh(geometry, material);
  overlay.position.copy(center);
  overlay.userData.baseColor = fallbackColor.clone();
  return overlay;
};

const heatmapValue = (asset, mode) => {
  const preset = HEATMAP_PRESETS[mode];
  if (!preset) return 0;
  const metrics = asset?.metrics || asset?.telemetry;
  if (!metrics || typeof metrics[mode] !== 'number') return 0;
  const value = (metrics[mode] - preset.min) / (preset.max - preset.min);
  return Math.min(1, Math.max(0, value));
};

const fitCameraToObjects = (camera, controls, objects) => {
  if (!objects.length) return;
  const box = new THREE.Box3();
  objects.forEach((object) => {
    const childBox = new THREE.Box3().setFromObject(object);
    box.union(childBox);
  });
  const size = new THREE.Vector3();
  const center = new THREE.Vector3();
  box.getSize(size);
  box.getCenter(center);

  const maxDim = Math.max(size.x, size.y, size.z);
  const fov = THREE.MathUtils.degToRad(camera.fov);
  const distance = Math.abs(maxDim / Math.sin(fov / 2));

  controls.target.copy(center);
  camera.position.set(center.x + distance * 0.4, center.y + distance * 0.6, center.z + distance * 0.6);
  camera.near = distance / 100;
  camera.far = distance * 100;
  camera.updateProjectionMatrix();
  controls.update();
};

export const initTwinScene = ({ twin, showToast }) => {
  const canvas = document.getElementById('twinCanvas');
  if (!canvas) return null;

  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
  renderer.setSize(canvas.clientWidth, canvas.clientHeight, false);
  renderer.setPixelRatio(window.devicePixelRatio || 1);
  renderer.shadowMap.enabled = true;

  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x101522);

  const camera = new THREE.PerspectiveCamera(50, canvas.clientWidth / canvas.clientHeight, 0.1, 2000);
  camera.position.set(160, 120, 160);

  const controls = new OrbitControls(camera, canvas);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.target.set(0, 15, 0);

  scene.add(new THREE.AmbientLight(0xffffff, 0.55));
  const dirLight = new THREE.DirectionalLight(0xffffff, 0.9);
  dirLight.position.set(140, 220, 160);
  dirLight.castShadow = true;
  dirLight.shadow.camera.near = 10;
  dirLight.shadow.camera.far = 800;
  dirLight.shadow.mapSize.set(4096, 4096);
  scene.add(dirLight);

  const overlayMeshes = new Map();
  const rootNodes = [];
  let heatmapMode = null;

  const loadEnvironment = async () => {
    try {
      const environment = await getModel('environment');
      if (!environment) return null;
      scene.add(environment);
      rootNodes.push(environment);
      return environment;
    } catch (error) {
      console.error('[twin] three load env', error);
      showToast?.('Jumeau', "Impossible de charger la scène 3D", 'danger');
      return null;
    }
  };

  const loadAsset = async ({ asset, canonical }, index) => {
    const friendlyName = FRIENDLY_LABELS[canonical];
    const canonicalAsset = {
      ...asset,
      asset_id: canonical,
      metadata: { ...(asset.metadata || {}), display_name: friendlyName },
      metrics: { ...(asset.metrics || {}), ...(asset.telemetry || {}) },
      telemetry: { ...(asset.metrics || {}), ...(asset.telemetry || {}) },
    };

    const pivot = new THREE.Group();
    const position = asset?.position || {};
    pivot.position.set(position.x ?? index * 30, position.y ?? 0, position.z ?? 0);
    scene.add(pivot);
    rootNodes.push(pivot);

    let object3D = null;
    if (canonical === 'SUP:PROV') {
      object3D = createCouveuseCube();
    } else {
      try {
        object3D = await getModel(canonical);
      } catch (error) {
        console.warn('[twin] three asset load', canonical, error);
      }
    }

    if (object3D) {
      pivot.add(object3D);
      rootNodes.push(object3D);
    }

    const overlayColor = DEFAULT_COLORS[canonical] || DEFAULT_COLORS.default;
    const overlay = computeOverlay(object3D || pivot, overlayColor);
    if (overlay) {
      overlay.userData.asset = canonicalAsset;
      overlayMeshes.set(canonical, overlay);
      pivot.add(overlay);
      rootNodes.push(overlay);
    }
  };

  const assets = [];
  (twin?.assets || []).forEach((asset) => {
    const canonical = canonicalAssetId(asset);
    if (!canonical || !FRIENDLY_LABELS[canonical]) return;
    if (assets.find((entry) => entry.canonical === canonical)) return;
    assets.push({ asset, canonical });
  });

  const loadingTasks = [loadEnvironment(), ...assets.map((entry, index) => loadAsset(entry, index))];

  Promise.allSettled(loadingTasks).then(() => {
    fitCameraToObjects(camera, controls, rootNodes);
  });

  const applyHeatmap = (mode) => {
    heatmapMode = mode;
    const preset = HEATMAP_PRESETS[mode];
    if (!preset) return;
    overlayMeshes.forEach((mesh) => {
      const asset = mesh.userData.asset;
      const value = heatmapValue(asset, mode);
      mesh.material.color.copy(preset.start.clone().lerp(preset.end, value));
    });
  };

  const clearHeatmap = () => {
    heatmapMode = null;
    overlayMeshes.forEach((mesh) => {
      mesh.material.color.copy(mesh.userData.baseColor || mesh.userData.asset?.baseColor || new THREE.Color('#6f9bff'));
    });
  };

  const focusAsset = (assetId) => {
    const mesh = overlayMeshes.get(assetId);
    if (!mesh) {
      showToast?.('Jumeau', `Élément « ${assetId} » introuvable`, 'warning');
      return;
    }
    const box = new THREE.Box3().setFromObject(mesh);
    const size = new THREE.Vector3();
    const center = new THREE.Vector3();
    box.getSize(size);
    box.getCenter(center);
    const maxDim = Math.max(size.x, size.y, size.z);
    const distance = maxDim * 2.5;
    controls.target.copy(center);
    camera.position.set(center.x + distance, center.y + distance, center.z + distance);
    camera.updateProjectionMatrix();
    controls.update();
  };

  const toggleHeatmap = (mode) => {
    if (!mode) return;
    if (heatmapMode === mode) clearHeatmap();
    else applyHeatmap(mode);
  };

  const syncAssets = (assetList = []) => {
    assetList.forEach((asset) => {
      const canonical = canonicalAssetId(asset);
      if (!canonical) return;
      const overlay = overlayMeshes.get(canonical);
      if (!overlay) return;
      const combined = { ...(asset.metrics || {}), ...(asset.telemetry || {}) };
      overlay.userData.asset = {
        ...asset,
        asset_id: canonical,
        metadata: { ...(asset.metadata || {}), display_name: FRIENDLY_LABELS[canonical] },
        metrics: combined,
        telemetry: combined,
      };
    });
    if (heatmapMode) applyHeatmap(heatmapMode);
  };

  const setLayoutMode = (active) => {
    overlayMeshes.forEach((mesh) => {
      mesh.material.opacity = active ? 0.4 : 0.25;
      mesh.material.transparent = true;
      mesh.material.needsUpdate = true;
    });
  };

  const animate = () => {
    renderer.setSize(canvas.clientWidth, canvas.clientHeight, false);
    const aspect = canvas.clientWidth / canvas.clientHeight;
    if (camera.aspect !== aspect) {
      camera.aspect = aspect;
      camera.updateProjectionMatrix();
    }
    controls.update();
    renderer.render(scene, camera);
  };

  renderer.setAnimationLoop(animate);

  window.addEventListener('resize', () => {
    renderer.setSize(canvas.clientWidth, canvas.clientHeight, false);
    camera.aspect = canvas.clientWidth / canvas.clientHeight;
    camera.updateProjectionMatrix();
  });

  return {
    toggleHeatmap,
    setLayoutMode,
    focusAsset,
    calibrate: () => undefined,
    saveLayout: () => undefined,
    refreshHeatmap: () => (heatmapMode ? applyHeatmap(heatmapMode) : undefined),
    syncAssets,
  };
};

export default initTwinScene;
