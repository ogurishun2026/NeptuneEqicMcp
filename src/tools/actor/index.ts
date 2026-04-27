import { Tool } from '../registry.js';
import { createActorTool } from './create.js';
import { deleteActorTool } from './delete.js';
import { findActorTool } from './find.js';
import { listActorsTool } from './list.js';
import { getTransformTool, setTransformTool } from './transform.js';
import { getPropertyTool, setPropertyTool } from './property.js';
import { addComponentTool, removeComponentTool, getComponentsTool } from './component.js';

export const actorTools: Tool[] = [
  createActorTool, deleteActorTool, findActorTool, listActorsTool,
  getTransformTool, setTransformTool, getPropertyTool, setPropertyTool,
  addComponentTool, removeComponentTool, getComponentsTool,
];

export * from './create.js';
export * from './delete.js';
export * from './find.js';
export * from './list.js';
export * from './transform.js';
export * from './property.js';
export * from './component.js';
