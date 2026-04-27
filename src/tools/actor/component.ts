import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorComponentParams, ActorRemoveComponentParams } from '../../types/commands.js';

interface ComponentInfo { componentId: string; name: string; className: string; }

export const addComponentTool: Tool = {
  name: 'actor_add_component',
  description: 'Add a component to an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: { type: 'string', description: 'ID of the actor' },
      componentType: { type: 'string', description: 'Type of component to add (e.g., "StaticMeshComponent", "PointLightComponent")' },
      name: { type: 'string', description: 'Name for the component (optional)' },
    },
    required: ['actorId', 'componentType'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as unknown as ActorComponentParams;
    const response = await context.connection.sendCommand('actor.addComponent', {
      actorId: typedParams.actorId, componentType: typedParams.componentType, name: typedParams.name,
    }) as { success: boolean; data?: { componentId: string; name: string }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to add component: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: `Added component "${response.data!.name}" with ID: ${response.data!.componentId}` }] };
  },
};

export const removeComponentTool: Tool = {
  name: 'actor_remove_component',
  description: 'Remove a component from an actor.',
  inputSchema: {
    type: 'object',
    properties: { actorId: { type: 'string', description: 'ID of the actor' }, componentId: { type: 'string', description: 'ID of the component to remove' } },
    required: ['actorId', 'componentId'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as unknown as ActorRemoveComponentParams;
    const response = await context.connection.sendCommand('actor.removeComponent', { actorId: typedParams.actorId, componentId: typedParams.componentId }) as { success: boolean; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to remove component: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: `Component "${typedParams.componentId}" removed successfully` }] };
  },
};

export const getComponentsTool: Tool = {
  name: 'actor_get_components',
  description: 'List all components attached to an actor.',
  inputSchema: { type: 'object', properties: { actorId: { type: 'string', description: 'ID of the actor' } }, required: ['actorId'] },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getComponents', { actorId: params.actorId }) as { success: boolean; data?: { components: ComponentInfo[] }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to get components: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    const components = response.data?.components ?? [];
    if (components.length === 0) {
      return { content: [{ type: 'text', text: 'No components found on this actor' }] };
    }
    const compList = components.map((c) => `  - ${c.name} (${c.className}, ID: ${c.componentId})`).join('\n');
    return { content: [{ type: 'text', text: `${components.length} component(s):\n${compList}` }] };
  },
};
