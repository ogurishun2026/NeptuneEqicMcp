import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorCreateParams } from '../../types/commands.js';

export const createActorTool: Tool = {
  name: 'actor_create',
  description: 'Create a new Actor in the Unreal scene. Returns the created actor ID and name.',
  inputSchema: {
    type: 'object',
    properties: {
      name: { type: 'string', description: 'Name for the new Actor' },
      location: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'World position [x, y, z]. Default: [0, 0, 0]' },
      rotation: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'Rotation in degrees [pitch, yaw, roll]. Default: [0, 0, 0]' },
      scale: { type: 'array', items: { type: 'number' }, minItems: 3, maxItems: 3, description: 'Scale [x, y, z]. Default: [1, 1, 1]' },
      actorClass: { type: 'string', description: 'Actor class to spawn (e.g., "StaticMeshActor", "PointLight"). Default: "Actor"' },
    },
    required: ['name'],
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as unknown as ActorCreateParams;
    const response = await context.connection.sendCommand('actor.create', {
      name: typedParams.name,
      location: typedParams.location ?? [0, 0, 0],
      rotation: typedParams.rotation ?? [0, 0, 0],
      scale: typedParams.scale ?? [1, 1, 1],
      actorClass: typedParams.actorClass ?? 'Actor',
    }) as { success: boolean; data?: { actorId: string; name: string }; error?: { message: string } };

    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to create actor: ${response.error?.message ?? 'Unknown error'}` }] };
    }

    const data = response.data as { actorId: string; name: string };
    return { content: [{ type: 'text', text: `Created actor "${data.name}" with ID: ${data.actorId}` }] };
  },
};
