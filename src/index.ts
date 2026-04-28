#!/usr/bin/env node

import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { ConnectionManager } from './core/connection.js';
import { EventDispatcher } from './core/eventDispatcher.js';
import { ToolRegistry } from './tools/registry.js';
import { createMCPServer } from './core/protocol.js';
import { actorTools } from './tools/actor/index.js';

async function main() {
  // Initialize components
  const connection = new ConnectionManager({
    host: process.env.UNREAL_HOST ?? 'localhost',
    port: parseInt(process.env.UNREAL_PORT ?? '18765', 10),
  });

  const eventDispatcher = new EventDispatcher();
  const registry = new ToolRegistry();

  // Register tools
  registry.registerAll(actorTools);

  // Handle events from Unreal
  connection.on('event', (event) => {
    eventDispatcher.dispatch(event);
  });

  // Create and start MCP server
  const server = await createMCPServer(registry, connection, eventDispatcher);
  const transport = new StdioServerTransport();

  await server.connect(transport);

  console.error('Unreal MCP Server started');
  console.error(`Connecting to Unreal at ${process.env.UNREAL_HOST ?? 'localhost'}:${process.env.UNREAL_PORT ?? '18765'}`);
}

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});
