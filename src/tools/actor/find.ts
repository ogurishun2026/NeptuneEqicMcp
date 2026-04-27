import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorFindParams } from '../../types/commands.js';

interface ActorInfo { actorId: string; name: string; className: string; }

export const findActorTool: Tool = {
  name: 'actor_find',
  description: 'Find actors by ID, name, class, or tag. Supports wildcard (*) in name search.',
  inputSchema: {
    type: 'object',
    properties: {
      byId: { type: 'string', description: 'Find by exact actor ID' },
      byName: { type: 'string', description: 'Find by name (supports wildcard *)' },
      byClass: { type: 'string', description: 'Find by actor class name' },
      byTag: { type: 'string', description: 'Find by actor tag' },
    },
  },
  async execute(params: Record<string, unknown>, context: ToolContext): Promise<ToolResult> {
    const typedParams = params as ActorFindParams;
    const commandParams: Record<string, unknown> = {};
    if (typedParams.byId) commandParams.byId = typedParams.byId;
    if (typedParams.byName) commandParams.byName = typedParams.byName;
    if (typedParams.byClass) commandParams.byClass = typedParams.byClass;
    if (typedParams.byTag) commandParams.byTag = typedParams.byTag;

    if (Object.keys(commandParams).length === 0) {
      return { content: [{ type: 'text', text: 'Error: At least one search criteria must be provided' }] };
    }

    const response = await context.connection.sendCommand('actor.find', commandParams) as { success: boolean; data?: { actors: ActorInfo[] }; error?: { message: string } };
    if (!response.success) {
      return { content: [{ type: 'text', text: `Failed to find actors: ${response.error?.message ?? 'Unknown error'}` }] };
    }

    const actors = response.data?.actors ?? [];
    if (actors.length === 0) {
      return { content: [{ type: 'text', text: 'No actors found matching the criteria' }] };
    }

    const actorList = actors.map((a) => `  - ${a.name} (ID: ${a.actorId}, Class: ${a.className})`).join('\n');
    return { content: [{ type: 'text', text: `Found ${actors.length} actor(s):\n${actorList}` }] };
  },
};
