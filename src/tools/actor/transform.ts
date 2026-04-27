import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorTransformParams, Vector3 } from '../../types/commands.js';

export const getTransformTool: Tool = {
  name: 'actor_get_transform',
  description: 'Get the transform (location, rotation, scale) of an actor.',
  inputSchema: { type: 'object', properties: { actorId: { type: 'string', description: 'ID of the actor' } }, required: ['actorId'] },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getTransform', { actorId: params.actorId }) as { success: boolean; data?: { location: Vector3; rotation: Vector3; scale: Vector3 }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to get transform: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    const data = response.data!;
    return { content: [{ type: 'text', text: `Transform:\n  Location: [${data.location.join(', ')}]\n  Rotation: [${data.rotation.join(', ')}]\n  Scale: [${data.scale.join(', ')}]` }] };
  },
};

export const setTransformTool: Tool = {
  name: 'actor_set_transform',
  description: 'Set the transform (location, rotation, scale) of an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: { type: 'string', description: 'ID of the actor' },
      location: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'New world position [x, y, z]' },
      rotation: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'New rotation in degrees [pitch, yaw, roll]' },
      scale: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'New scale [x, y, z]' },
    },
    required: ['actorId'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as unknown as ActorTransformParams;
    const commandParams: Record<string, unknown> = { actorId: typedParams.actorId };
    if (typedParams.location) commandParams.location = typedParams.location;
    if (typedParams.rotation) commandParams.rotation = typedParams.rotation;
    if (typedParams.scale) commandParams.scale = typedParams.scale;

    const response = await context.connection.sendCommand('actor.setTransform', commandParams) as { success: boolean; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to set transform: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: 'Transform updated successfully' }] };
  },
};
