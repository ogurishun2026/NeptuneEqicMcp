import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorPropertyParams } from '../../types/commands.js';

export const getPropertyTool: Tool = {
  name: 'actor_get_property',
  description: 'Get a property value from an actor.',
  inputSchema: {
    type: 'object',
    properties: { actorId: { type: 'string', description: 'ID of the actor' }, propertyName: { type: 'string', description: 'Name of the property to get' } },
    required: ['actorId', 'propertyName'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getProperty', { actorId: params.actorId, propertyName: params.propertyName }) as { success: boolean; data?: { value: unknown }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to get property: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: `Property "${params.propertyName}": ${JSON.stringify(response.data?.value)}` }] };
  },
};

export const setPropertyTool: Tool = {
  name: 'actor_set_property',
  description: 'Set a property value on an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: { type: 'string', description: 'ID of the actor' },
      propertyName: { type: 'string', description: 'Name of the property to set' },
      propertyType: { type: 'string', enum: ['int', 'float', 'string', 'bool', 'vector', 'rotator', 'color'], description: 'Type of the property value' },
      value: { description: 'Value to set (type depends on propertyType)' },
    },
    required: ['actorId', 'propertyName', 'propertyType', 'value'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as unknown as ActorPropertyParams;
    const response = await context.connection.sendCommand('actor.setProperty', {
      actorId: typedParams.actorId, propertyName: typedParams.propertyName, propertyType: typedParams.propertyType, value: typedParams.value,
    }) as { success: boolean; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to set property: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: `Property "${typedParams.propertyName}" set successfully` }] };
  },
};
