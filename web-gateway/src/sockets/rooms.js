export const roomFromScope = ({ site, building, zone, asset }) => {
  if (asset) return `asset:${asset}`;
  if (zone) return `zone:${zone}`;
  if (building) return `building:${building}`;
  if (site) return `site:${site}`;
  return 'site:SITE:KIN-GOLIATH';
};

export const computeUserRooms = (user) => {
  if (!user?.scopes) return ['site:SITE:KIN-GOLIATH'];
  const rooms = new Set();
  if (user.scopes.site) user.scopes.site.forEach((s) => rooms.add(`site:${s}`));
  if (user.scopes.building) user.scopes.building.forEach((b) => rooms.add(`building:${b}`));
  if (user.scopes.zone) user.scopes.zone.forEach((z) => rooms.add(`zone:${z}`));
  if (user.scopes.lot) user.scopes.lot.forEach((l) => rooms.add(`lot:${l}`));
  rooms.add('dashboard');
  return Array.from(rooms);
};
