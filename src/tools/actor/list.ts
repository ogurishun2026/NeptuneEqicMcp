import { Tool, ToolContext, ToolResult } from '../registry.js';

interface ActorInfo { actorId: string; name: string; className: string; }

export const listActorsTool: Tool = {
  name: 'actor_list',
  description: 'List all actors in the current level. Optionally filter by class.',
  inputSchema: {
    type: 'object',
    properties: {
      filter: { type: 'object', properties: { class: { type: 'string', description: 'Filter by actor class name' } } },
    },
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.list', { filter: params.filter }) as { success: boolean; data?: { actors: ActorInfo[] }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to list actors: ${response.error?.message ?? 'Unknown error'}` }] };
    }

    const actors = response.data?.actors ?? [];
    if (actors.length === 0) {
      return { content: [{ type: 'text', text: 'No actors found in the level' }] };
    }

    const actorList = actors.map((a) => `  - ${a.name} (${a.className}, ID: ${a.actorId})`).join('\n');
    return { content: [{ type: 'text', text: `${actors.length} actor(s) in level:\n${actorList}` }] };
  },
};
