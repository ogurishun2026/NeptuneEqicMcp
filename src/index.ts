#!/usr/bin/env node

import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { ConnectionManager } from './core/connection.js';
import { CloudServer } from './core/cloudServer.js';
import { EventDispatcher } from './core/eventDispatcher.js';
import { ToolRegistry } from './tools/registry.js';
import { createMCPServer, createCloudMCPServer } from './core/protocol.js';
import { actorTools } from './tools/actor/index.js';
import * as fs from 'fs';
import * as path from 'path';

interface McpConfig {
  mode: 'local' | 'cloud';
  local: { host: string; port: number };
  cloud: { host: string; port: number };
}

function loadConfig(): McpConfig {
  // 优先使用环境变量，否则读取配置文件
  const mode = process.env.MCP_MODE as 'local' | 'cloud' | undefined;

  if (mode) {
    // 环境变量模式
    if (mode === 'cloud') {
      return {
        mode: 'cloud',
        local: { host: 'localhost', port: 18765 },
        cloud: {
          host: process.env.MCP_CLOUD_HOST ?? 'localhost',
          port: parseInt(process.env.MCP_CLOUD_PORT ?? '18765', 10),
        },
      };
    } else {
      return {
        mode: 'local',
        local: {
          host: process.env.UNREAL_HOST ?? 'localhost',
          port: parseInt(process.env.UNREAL_PORT ?? '18765', 10),
        },
        cloud: { host: 'localhost', port: 18765 },
      };
    }
  }

  // 读取配置文件
  const configPath = path.join(process.cwd(), 'mcp-config.json');
  try {
    const configData = fs.readFileSync(configPath, 'utf-8');
    return JSON.parse(configData) as McpConfig;
  } catch {
    // 默认本地模式
    return {
      mode: 'local',
      local: { host: 'localhost', port: 18765 },
      cloud: { host: 'localhost', port: 18765 },
    };
  }
}

async function main() {
  const config = loadConfig();
  const eventDispatcher = new EventDispatcher();
  const registry = new ToolRegistry();
  registry.registerAll(actorTools);

  console.error(`NeptuneEqicMcp starting in ${config.mode} mode`);

  if (config.mode === 'local') {
    // 本地模式：作为客户端连接 Unreal
    const connection = new ConnectionManager({
      host: config.local.host,
      port: config.local.port,
    });

    connection.on('event', (event) => {
      eventDispatcher.dispatch(event);
    });

    const server = await createMCPServer(registry, connection, eventDispatcher);
    const transport = new StdioServerTransport();
    await server.connect(transport);

    console.error(`Local mode: Connecting to Unreal at ${config.local.host}:${config.local.port}`);
  } else {
    // 云端模式：作为服务器等待 Unreal 连接
    const cloudServer = new CloudServer({
      port: config.cloud.port,
    });

    cloudServer.on('event', (event) => {
      eventDispatcher.dispatch(event);
    });

    await cloudServer.start();

    const server = await createCloudMCPServer(registry, cloudServer, eventDispatcher);
    const transport = new StdioServerTransport();
    await server.connect(transport);

    console.error(`Cloud mode: Listening on port ${config.cloud.port}, waiting for Unreal to connect`);
  }
}

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});
