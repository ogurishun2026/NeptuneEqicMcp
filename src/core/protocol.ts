import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  type CallToolResult,
  type ListToolsResult,
} from '@modelcontextprotocol/sdk/types.js';
import { ToolRegistry } from '../tools/registry.js';
import { ConnectionManager } from './connection.js';
import { EventDispatcher } from './eventDispatcher.js';

export interface MCPServerOptions {
  host: string;
  port: number;
}

export async function createMCPServer(
  registry: ToolRegistry,
  connection: ConnectionManager,
  eventDispatcher: EventDispatcher
): Promise<Server> {
  const server = new Server(
    { name: 'unreal-mcp-server', version: '1.0.0' },
    { capabilities: { tools: {} } }
  );

  // List tools handler
  server.setRequestHandler(ListToolsRequestSchema, async (): Promise<ListToolsResult> => {
    const tools = registry.getAll().map((tool) => ({
      name: tool.name,
      description: tool.description,
      inputSchema: tool.inputSchema,
    }));

    return { tools };
  });

  // Call tool handler
  server.setRequestHandler(CallToolRequestSchema, async (request): Promise<CallToolResult> => {
    const { name, arguments: args } = request.params;
    const tool = registry.get(name);

    if (!tool) {
      return {
        content: [{ type: 'text', text: `Unknown tool: ${name}` }],
        isError: true,
      };
    }

    try {
      // Ensure connection
      if (!connection.isConnected()) {
        await connection.connect();
      }

      const context = {
        connection: {
          sendCommand: (cmd: string, params: Record<string, unknown>) =>
            connection.sendCommand(cmd, params),
        },
      };

      const result = await tool.execute(args ?? {}, context);
      return result as CallToolResult;
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Error executing ${name}: ${error instanceof Error ? error.message : String(error)}`,
          },
        ],
        isError: true,
      };
    }
  });

  return server;
}
