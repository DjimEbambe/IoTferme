import { permissions } from './permissions.js';

export const rolePermissions = {
  admin: Object.values(permissions),
  manager: [
    permissions.VIEW_DASHBOARD,
    permissions.VIEW_KPI,
    permissions.VIEW_ERP,
    permissions.VIEW_TWIN,
    permissions.MANAGE_ERP,
    permissions.MANAGE_MAINTENANCE,
    permissions.MANAGE_SALES,
    permissions.ISSUE_COMMAND,
    permissions.MANAGE_TWIN,
    permissions.VIEW_REPORTS,
  ],
  technicien: [
    permissions.VIEW_DASHBOARD,
    permissions.VIEW_KPI,
    permissions.VIEW_ERP,
    permissions.VIEW_TWIN,
    permissions.ISSUE_COMMAND,
    permissions.MANAGE_TWIN,
    permissions.MANAGE_MAINTENANCE,
  ],
  veterinaire: [
    permissions.VIEW_DASHBOARD,
    permissions.VIEW_KPI,
    permissions.VIEW_ERP,
    permissions.MANAGE_HEALTH,
    permissions.VIEW_REPORTS,
  ],
  visiteur: [permissions.VIEW_DASHBOARD, permissions.VIEW_KPI],
};

export const rolePriority = ['visiteur', 'veterinaire', 'technicien', 'manager', 'admin'];
