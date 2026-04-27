import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorDeleteParams } from '../../types/commands.js';

export const deleteActorTool: Tool = {
  name: 'actor_delete',
  description: 'Delete an Actor from the scene by ID or name.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: { type: 'string', description: 'ID of the actor to delete' },
      name: { type: 'string', description: 'Name of the actor to delete (alternative to actorId)' },
    },
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as ActorDeleteParams;
    if (!typedParams.actorId && !typedParams.name) {
      return { content: [{ type: 'text', text: 'Error: Either actorId or name must be provided' }] };
    }

    const commandParams: Record<string, unknown> = {};
    if (typedParams.actorId) commandParams.actorId = typedParams.actorId;
    if (typedParams.name) commandParams.name = typedParams.name;

    const response = await context.connection.sendCommand('actor.delete', commandParams) as { success: boolean; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to delete actor: ${response.error?.message ?? 'Unknown error'}` }] };
    }
    return { content: [{ type: 'text', text: 'Actor deleted successfully' }] };
  },
};
