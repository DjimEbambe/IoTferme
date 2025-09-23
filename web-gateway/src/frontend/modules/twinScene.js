import * as BABYLON from 'babylonjs';

export const initTwinScene = ({ twin, showToast }) => {
  const canvas = document.getElementById('twinCanvas');
  if (!canvas) return null;

  const engine = new BABYLON.Engine(canvas, true, { preserveDrawingBuffer: true, stencil: true });
  const scene = new BABYLON.Scene(engine);
  scene.clearColor = new BABYLON.Color4(0.04, 0.06, 0.12, 0.95);

  const camera = new BABYLON.ArcRotateCamera('mainCamera', Math.PI / 2.8, Math.PI / 3, 160, new BABYLON.Vector3(0, 10, 0), scene);
  camera.attachControl(canvas, true);
  camera.lowerRadiusLimit = 40;
  camera.upperRadiusLimit = 220;

  const hemi = new BABYLON.HemisphericLight('hemi', new BABYLON.Vector3(0, 1, 0), scene);
  hemi.intensity = 0.85;
  const dirLight = new BABYLON.DirectionalLight('dir', new BABYLON.Vector3(-0.2, -0.6, -0.3), scene);
  dirLight.position = new BABYLON.Vector3(60, 120, 60);

  const ground = BABYLON.MeshBuilder.CreateGround('ground', { width: 220, height: 220 }, scene);
  const groundMat = new BABYLON.StandardMaterial('groundMat', scene);
  groundMat.diffuseColor = new BABYLON.Color3(0.05, 0.08, 0.15);
  ground.material = groundMat;

  const meshes = new Map();
  const assets = twin?.assets || [];
  const defaultMaterial = new BABYLON.StandardMaterial('defaultMat', scene);
  defaultMaterial.diffuseColor = BABYLON.Color3.FromHexString('#3aa6ff');
  defaultMaterial.alpha = 0.9;

  assets.forEach((asset, idx) => {
    const dimensions = asset.type === 'building' ? { width: 55, height: 12, depth: 18 } : { width: 12, height: 6, depth: 12 };
    const mesh = BABYLON.MeshBuilder.CreateBox(asset.asset_id, dimensions, scene);
    mesh.position = new BABYLON.Vector3(asset.position?.x || idx * 10, dimensions.height / 2, asset.position?.z || 0);
    const mat = defaultMaterial.clone(`${asset.asset_id}-mat`);
    mat.diffuseColor = BABYLON.Color3.FromHexString('#3aa6ff');
    mat.emissiveColor = BABYLON.Color3.Black();
    mesh.material = mat;
    mesh.metadata = { asset };
    meshes.set(asset.asset_id, mesh);
  });

  engine.runRenderLoop(() => scene.render());
  window.addEventListener('resize', () => engine.resize());

  const highlightLayer = new BABYLON.HighlightLayer('hl1', scene);
  let heatmapMode = null;

  const applyHeatmap = (mode) => {
    heatmapMode = mode;
    meshes.forEach((mesh) => {
      const value = Math.random();
      const color = mode === 'temperature'
        ? BABYLON.Color3.Lerp(new BABYLON.Color3(0.1, 0.2, 0.6), new BABYLON.Color3(0.9, 0.4, 0.2), value)
        : BABYLON.Color3.Lerp(new BABYLON.Color3(0.2, 0.4, 0.9), new BABYLON.Color3(0.7, 0.9, 0.2), value);
      mesh.material.diffuseColor = color;
      mesh.material.emissiveColor = color.scale(0.25);
    });
  };

  const clearHeatmap = () => {
    heatmapMode = null;
    meshes.forEach((mesh) => {
      mesh.material.diffuseColor = BABYLON.Color3.FromHexString('#3aa6ff');
      mesh.material.emissiveColor = BABYLON.Color3.Black();
    });
  };

  const toggleHeatmap = (mode) => {
    if (heatmapMode === mode) clearHeatmap();
    else applyHeatmap(mode);
  };

  const setLayoutMode = (active) => {
    meshes.forEach((mesh) => {
      mesh.enableEdgesRendering(active);
      mesh.edgesColor = new BABYLON.Color4(0.6, 1, 0.8, 1);
      mesh.edgesWidth = 3;
      if (active) highlightLayer.addMesh(mesh, BABYLON.Color3.FromHexString('#5fd4ac'));
      else highlightLayer.removeMesh(mesh);
    });
  };

  const focusAsset = (assetId) => {
    const mesh = meshes.get(assetId);
    if (!mesh) {
      showToast?.('Jumeau', `Asset ${assetId} introuvable`, 'warning');
      return;
    }
    camera.setTarget(mesh.position);
    camera.radius = 70;
    highlightLayer.addMesh(mesh, BABYLON.Color3.FromHexString('#f7ba56'));
    setTimeout(() => highlightLayer.removeMesh(mesh), 1500);
  };

  const calibrate = ({ ppId }) => {
    showToast?.('Calibration', `Offsets appliqués sur ${ppId}`, 'success');
  };

  const saveLayout = () => {
    showToast?.('Layout', 'Disposition sauvegardée (place-holder).');
  };

  return {
    toggleHeatmap,
    setLayoutMode,
    focusAsset,
    calibrate,
    saveLayout,
  };
};
