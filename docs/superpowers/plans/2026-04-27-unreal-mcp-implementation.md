# Unreal MCP Server Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建Unreal Engine MCP服务器，支持通过Claude操作Unreal编辑器中的Actor。

**Architecture:** TypeScript MCP服务器通过WebSocket与Unreal C++插件通信，采用工具注册模式实现可扩展架构。

**Tech Stack:** TypeScript, Node.js, @modelcontextprotocol/sdk, WebSocket, C++ (Unreal Engine 5.6-5.7)

---

## Phase 1: MCP Server 基础设施

### Task 1: 初始化项目结构

**Files:**
- Create: `D:/unreal-mcp-server/package.json`
- Create: `D:/unreal-mcp-server/tsconfig.json`
- Create: `D:/unreal-mcp-server/src/index.ts`
- Create: `D:/unreal-mcp-server/.gitignore`

- [ ] **Step 1: 创建 package.json**

```json
{
  "name": "unreal-mcp-server",
  "version": "1.0.0",
  "description": "MCP server for Unreal Engine integration",
  "type": "module",
  "main": "dist/index.js",
  "bin": {
    "unreal-mcp-server": "dist/index.js"
  },
  "scripts": {
    "build": "tsc",
    "start": "node dist/index.js",
    "dev": "tsc --watch",
    "test": "vitest"
  },
  "dependencies": {
    "@modelcontextprotocol/sdk": "^1.0.0",
    "ws": "^8.16.0",
    "uuid": "^9.0.0"
  },
  "devDependencies": {
    "@types/node": "^20.10.0",
    "@types/ws": "^8.5.10",
    "@types/uuid": "^9.0.7",
    "typescript": "^5.3.0",
    "vitest": "^1.0.0"
  }
}
```

- [ ] **Step 2: 创建 tsconfig.json**

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "NodeNext",
    "moduleResolution": "NodeNext",
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "declaration": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

- [ ] **Step 3: 创建 .gitignore**

```
node_modules/
dist/
*.log
.env
.DS_Store
```

- [ ] **Step 4: 创建入口文件 src/index.ts**

```typescript
#!/usr/bin/env node

console.log('Unreal MCP Server - Initializing...');

async function main() {
  console.log('Unreal MCP Server started');
}

main().catch(console.error);
```

- [ ] **Step 5: 安装依赖并验证**

Run: `cd /d/unreal-mcp-server && npm install`
Expected: 依赖安装成功

Run: `npm run build`
Expected: 编译成功，dist/index.js 生成

- [ ] **Step 6: 初始化Git仓库**

Run: `cd /d/unreal-mcp-server && git init && git add . && git commit -m "chore: initialize project structure"`

---

### Task 2: 类型定义

**Files:**
- Create: `D:/unreal-mcp-server/src/types/commands.ts`
- Create: `D:/unreal-mcp-server/src/types/events.ts`
- Create: `D:/unreal-mcp-server/src/types/errors.ts`
- Create: `D:/unreal-mcp-server/src/types/index.ts`

- [ ] **Step 1: 创建命令类型 src/types/commands.ts**

```typescript
export interface Command {
  id: string;
  cmd: string;
  params: Record<string, unknown>;
}

export interface CommandResponse {
  id: string;
  success: boolean;
  data?: Record<string, unknown>;
  error?: ErrorResponse;
}

export type Vector3 = [number, number, number];

// Actor Commands
export interface ActorCreateParams {
  name: string;
  location?: Vector3;
  rotation?: Vector3;
  scale?: Vector3;
  actorClass?: string;
}

export interface ActorDeleteParams {
  actorId?: string;
  name?: string;
}

export interface ActorFindParams {
  byId?: string;
  byName?: string;
  byClass?: string;
  byTag?: string;
}

export interface ActorTransformParams {
  actorId: string;
  location?: Vector3;
  rotation?: Vector3;
  scale?: Vector3;
}

export interface ActorPropertyParams {
  actorId: string;
  propertyName: string;
  propertyType: 'int' | 'float' | 'string' | 'bool' | 'vector' | 'rotator' | 'color';
  value: unknown;
}

export interface ActorComponentParams {
  actorId: string;
  componentType: string;
  name?: string;
}

export interface ActorRemoveComponentParams {
  actorId: string;
  componentId: string;
}
```

- [ ] **Step 2: 创建事件类型 src/types/events.ts**

```typescript
export interface UnrealEvent {
  event: string;
  timestamp: string;
  data: Record<string, unknown>;
}

export type EventHandler = (event: UnrealEvent) => void;

export const EventTypes = {
  ACTOR_CREATED: 'actor.created',
  ACTOR_DELETED: 'actor.deleted',
  ACTOR_MODIFIED: 'actor.modified',
  PLAY_MODE_STARTED: 'play_mode.started',
  PLAY_MODE_ENDED: 'play_mode.ended',
  ASSET_IMPORTED: 'asset.imported',
  ERROR: 'error',
} as const;
```

- [ ] **Step 3: 创建错误类型 src/types/errors.ts**

```typescript
export interface ErrorResponse {
  code: string;
  type: ErrorType;
  message: string;
  stack?: string;
}

export type ErrorType =
  | 'validation_error'
  | 'not_found'
  | 'runtime_error'
  | 'connection_error'
  | 'timeout_error'
  | 'permission_error';

export const ErrorCodes = {
  ACTOR_NOT_FOUND: 'ACTOR_NOT_FOUND',
  ACTOR_SPAWN_FAILED: 'ACTOR_SPAWN_FAILED',
  ACTOR_DELETE_FAILED: 'ACTOR_DELETE_FAILED',
  INVALID_PARAMS: 'INVALID_PARAMS',
  CONNECTION_FAILED: 'CONNECTION_FAILED',
  COMMAND_TIMEOUT: 'COMMAND_TIMEOUT',
  UNKNOWN_COMMAND: 'UNKNOWN_COMMAND',
} as const;

export function createError(
  code: string,
  type: ErrorType,
  message: string,
  stack?: string
): ErrorResponse {
  return { code, type, message, stack };
}
```

- [ ] **Step 4: 创建类型索引 src/types/index.ts**

```typescript
export * from './commands.js';
export * from './events.js';
export * from './errors.js';
```

- [ ] **Step 5: 验证编译**

Run: `cd /d/unreal-mcp-server && npm run build`
Expected: 编译成功

- [ ] **Step 6: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: add type definitions for commands, events, and errors"`

---

### Task 3: WebSocket连接管理器

**Files:**
- Create: `D:/unreal-mcp-server/src/core/connection.ts`
- Create: `D:/unreal-mcp-server/src/core/__tests__/connection.test.ts`

- [ ] **Step 1: 创建测试文件 src/core/__tests__/connection.test.ts**

```typescript
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { ConnectionManager } from '../connection.js';

describe('ConnectionManager', () => {
  let manager: ConnectionManager;

  beforeEach(() => {
    manager = new ConnectionManager({ host: 'localhost', port: 8080 });
  });

  afterEach(async () => {
    await manager.disconnect();
  });

  it('should start in disconnected state', () => {
    expect(manager.isConnected()).toBe(false);
  });

  it('should emit state change events', () => {
    const listener = vi.fn();
    manager.on('stateChange', listener);
    manager.emit('stateChange', 'connected');
    expect(listener).toHaveBeenCalledWith('connected');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL - ConnectionManager not defined

- [ ] **Step 3: 实现连接管理器 src/core/connection.ts**

```typescript
import WebSocket from 'ws';
import { EventEmitter } from 'events';
import { Command, CommandResponse } from '../types/index.js';
import { v4 as uuidv4 } from 'uuid';

export type ConnectionState = 'connecting' | 'connected' | 'disconnecting' | 'disconnected';

export interface ConnectionOptions {
  host: string;
  port: number;
  reconnectInterval?: number;
  commandTimeout?: number;
}

export class ConnectionManager extends EventEmitter {
  private ws: WebSocket | null = null;
  private state: ConnectionState = 'disconnected';
  private readonly options: Required<ConnectionOptions>;
  private pendingCommands: Map<string, {
    resolve: (response: CommandResponse) => void;
    reject: (error: Error) => void;
    timeout: NodeJS.Timeout;
  }> = new Map();

  constructor(options: ConnectionOptions) {
    super();
    this.options = {
      reconnectInterval: 1000,
      commandTimeout: 30000,
      ...options,
    };
  }

  getState(): ConnectionState {
    return this.state;
  }

  isConnected(): boolean {
    return this.state === 'connected' && this.ws?.readyState === WebSocket.OPEN;
  }

  async connect(): Promise<void> {
    if (this.isConnected()) return;

    this.setState('connecting');

    return new Promise((resolve, reject) => {
      const url = `ws://${this.options.host}:${this.options.port}`;
      this.ws = new WebSocket(url);

      this.ws.on('open', () => {
        this.setState('connected');
        resolve();
      });

      this.ws.on('message', (data: Buffer) => {
        this.handleMessage(data);
      });

      this.ws.on('close', () => {
        this.setState('disconnected');
        this.rejectAllPending('Connection closed');
      });

      this.ws.on('error', (error: Error) => {
        if (this.state === 'connecting') {
          reject(error);
        }
        this.setState('disconnected');
      });
    });
  }

  async disconnect(): Promise<void> {
    if (!this.ws) return;

    this.setState('disconnecting');
    this.ws.close();
    this.ws = null;
    this.setState('disconnected');
  }

  async sendCommand(cmd: string, params: Record<string, unknown>): Promise<CommandResponse> {
    if (!this.isConnected()) {
      await this.connect();
    }

    const id = uuidv4();
    const command: Command = { id, cmd, params };

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingCommands.delete(id);
        reject(new Error(`Command timeout: ${cmd}`));
      }, this.options.commandTimeout);

      this.pendingCommands.set(id, { resolve, reject, timeout });

      this.ws!.send(JSON.stringify(command));
    });
  }

  private handleMessage(data: Buffer): void {
    try {
      const message = JSON.parse(data.toString());

      // Check if it's a command response
      if (message.id !== undefined) {
        const pending = this.pendingCommands.get(message.id);
        if (pending) {
          clearTimeout(pending.timeout);
          this.pendingCommands.delete(message.id);
          pending.resolve(message as CommandResponse);
        }
      } else if (message.event !== undefined) {
        // It's an event from Unreal
        this.emit('event', message);
      }
    } catch (error) {
      this.emit('error', error);
    }
  }

  private setState(state: ConnectionState): void {
    this.state = state;
    this.emit('stateChange', state);
  }

  private rejectAllPending(reason: string): void {
    for (const [id, pending] of this.pendingCommands) {
      clearTimeout(pending.timeout);
      pending.reject(new Error(reason));
    }
    this.pendingCommands.clear();
  }
}
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement WebSocket connection manager"`

---

### Task 4: 事件分发器

**Files:**
- Create: `D:/unreal-mcp-server/src/core/eventDispatcher.ts`
- Create: `D:/unreal-mcp-server/src/core/__tests__/eventDispatcher.test.ts`

- [ ] **Step 1: 创建测试文件 src/core/__tests__/eventDispatcher.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { EventDispatcher } from '../eventDispatcher.js';
import { UnrealEvent, EventTypes } from '../../types/index.js';

describe('EventDispatcher', () => {
  it('should dispatch events to registered handlers', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    dispatcher.on(EventTypes.ACTOR_DELETED, handler);

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_DELETED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).toHaveBeenCalledWith(event);
  });

  it('should support wildcard handlers', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    dispatcher.on('*', handler);

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_CREATED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).toHaveBeenCalledWith(event);
  });

  it('should allow unsubscribing', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    const unsubscribe = dispatcher.on(EventTypes.ACTOR_DELETED, handler);
    unsubscribe();

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_DELETED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).not.toHaveBeenCalled();
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL - EventDispatcher not defined

- [ ] **Step 3: 实现事件分发器 src/core/eventDispatcher.ts**

```typescript
import { UnrealEvent, EventHandler } from '../types/index.js';

type Unsubscribe = () => void;

export class EventDispatcher {
  private handlers: Map<string, Set<EventHandler>> = new Map();

  on(eventType: string, handler: EventHandler): Unsubscribe {
    if (!this.handlers.has(eventType)) {
      this.handlers.set(eventType, new Set());
    }
    this.handlers.get(eventType)!.add(handler);

    return () => {
      this.handlers.get(eventType)?.delete(handler);
    };
  }

  dispatch(event: UnrealEvent): void {
    // Dispatch to specific handlers
    const specificHandlers = this.handlers.get(event.event);
    if (specificHandlers) {
      for (const handler of specificHandlers) {
        handler(event);
      }
    }

    // Dispatch to wildcard handlers
    const wildcardHandlers = this.handlers.get('*');
    if (wildcardHandlers) {
      for (const handler of wildcardHandlers) {
        handler(event);
      }
    }
  }

  clear(): void {
    this.handlers.clear();
  }
}
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement event dispatcher"`

---

### Task 5: 工具注册中心

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/registry.ts`
- Create: `D:/unreal-mcp-server/src/tools/__tests__/registry.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/__tests__/registry.test.ts**

```typescript
import { describe, it, expect } from 'vitest';
import { ToolRegistry, Tool } from '../registry.js';

describe('ToolRegistry', () => {
  it('should register and retrieve tools', () => {
    const registry = new ToolRegistry();
    const tool: Tool = {
      name: 'test_tool',
      description: 'A test tool',
      inputSchema: {
        type: 'object',
        properties: { param: { type: 'string' } },
      },
      execute: async () => ({ result: 'ok' }),
    };

    registry.register(tool);

    const tools = registry.getAll();
    expect(tools).toHaveLength(1);
    expect(tools[0].name).toBe('test_tool');
  });

  it('should find tool by name', () => {
    const registry = new ToolRegistry();
    const tool: Tool = {
      name: 'test_tool',
      description: 'A test tool',
      inputSchema: { type: 'object', properties: {} },
      execute: async () => ({ result: 'ok' }),
    };

    registry.register(tool);

    const found = registry.get('test_tool');
    expect(found).toBeDefined();
    expect(found?.name).toBe('test_tool');
  });

  it('should return undefined for unknown tool', () => {
    const registry = new ToolRegistry();
    expect(registry.get('unknown')).toBeUndefined();
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL - ToolRegistry not defined

- [ ] **Step 3: 实现工具注册中心 src/tools/registry.ts**

```typescript
export interface Tool {
  name: string;
  description: string;
  inputSchema: {
    type: 'object';
    properties: Record<string, unknown>;
    required?: string[];
  };
  execute: (params: Record<string, unknown>, context: ToolContext) => Promise<ToolResult>;
}

export interface ToolContext {
  connection: { sendCommand: (cmd: string, params: Record<string, unknown>) => Promise<unknown> };
}

export interface ToolResult {
  content: Array<{
    type: 'text';
    text: string;
  }>;
}

export class ToolRegistry {
  private tools: Map<string, Tool> = new Map();

  register(tool: Tool): void {
    this.tools.set(tool.name, tool);
  }

  registerAll(tools: Tool[]): void {
    for (const tool of tools) {
      this.register(tool);
    }
  }

  get(name: string): Tool | undefined {
    return this.tools.get(name);
  }

  getAll(): Tool[] {
    return Array.from(this.tools.values());
  }

  getNames(): string[] {
    return Array.from(this.tools.keys());
  }
}
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement tool registry"`

---

## Phase 2: Actor工具集

### Task 6: Actor工具 - create

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/create.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/create.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/create.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { createActorTool } from '../create.js';

describe('actor_create tool', () => {
  it('should have correct name and description', () => {
    const tool = createActorTool;
    expect(tool.name).toBe('actor_create');
    expect(tool.description).toContain('Create a new Actor');
  });

  it('should send correct command to Unreal', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: { actorId: 'actor-001', name: 'TestCube' },
      }),
    };

    const tool = createActorTool;
    const result = await tool.execute(
      { name: 'TestCube', location: [0, 0, 0] },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.create', {
      name: 'TestCube',
      location: [0, 0, 0],
    });
    expect(result.content[0].text).toContain('actor-001');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL - createActorTool not defined

- [ ] **Step 3: 实现 actor_create 工具 src/tools/actor/create.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorCreateParams } from '../../types/commands.js';

export const createActorTool: Tool = {
  name: 'actor_create',
  description: 'Create a new Actor in the Unreal scene. Returns the created actor ID and name.',
  inputSchema: {
    type: 'object',
    properties: {
      name: {
        type: 'string',
        description: 'Name for the new Actor',
      },
      location: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'World position [x, y, z]. Default: [0, 0, 0]',
      },
      rotation: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'Rotation in degrees [pitch, yaw, roll]. Default: [0, 0, 0]',
      },
      scale: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'Scale [x, y, z]. Default: [1, 1, 1]',
      },
      actorClass: {
        type: 'string',
        description: 'Actor class to spawn (e.g., "StaticMeshActor", "PointLight"). Default: "Actor"',
      },
    },
    required: ['name'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorCreateParams;
    const response = await context.connection.sendCommand('actor.create', {
      name: typedParams.name,
      location: typedParams.location ?? [0, 0, 0],
      rotation: typedParams.rotation ?? [0, 0, 0],
      scale: typedParams.scale ?? [1, 1, 1],
      actorClass: typedParams.actorClass ?? 'Actor',
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to create actor: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { actorId: string; name: string };
    return {
      content: [
        {
          type: 'text',
          text: `Created actor "${data.name}" with ID: ${data.actorId}`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor_create tool"`

---

### Task 7: Actor工具 - delete

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/delete.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/delete.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/delete.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { deleteActorTool } from '../delete.js';

describe('actor_delete tool', () => {
  it('should have correct name and description', () => {
    expect(deleteActorTool.name).toBe('actor_delete');
    expect(deleteActorTool.description).toContain('Delete an Actor');
  });

  it('should send command with actorId', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: { actorId: 'actor-001' },
      }),
    };

    await deleteActorTool.execute(
      { actorId: 'actor-001' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.delete', {
      actorId: 'actor-001',
    });
  });

  it('should send command with name', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: { name: 'MyCube' },
      }),
    };

    await deleteActorTool.execute(
      { name: 'MyCube' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.delete', {
      name: 'MyCube',
    });
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 actor_delete 工具 src/tools/actor/delete.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorDeleteParams } from '../../types/commands.js';

export const deleteActorTool: Tool = {
  name: 'actor_delete',
  description: 'Delete an Actor from the scene by ID or name.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor to delete',
      },
      name: {
        type: 'string',
        description: 'Name of the actor to delete (alternative to actorId)',
      },
    },
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorDeleteParams;

    if (!typedParams.actorId && !typedParams.name) {
      return {
        content: [
          {
            type: 'text',
            text: 'Error: Either actorId or name must be provided',
          },
        ],
      };
    }

    const commandParams: Record<string, unknown> = {};
    if (typedParams.actorId) commandParams.actorId = typedParams.actorId;
    if (typedParams.name) commandParams.name = typedParams.name;

    const response = await context.connection.sendCommand('actor.delete', commandParams);

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to delete actor: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    return {
      content: [
        {
          type: 'text',
          text: `Actor deleted successfully`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor_delete tool"`

---

### Task 8: Actor工具 - find

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/find.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/find.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/find.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { findActorTool } from '../find.js';

describe('actor_find tool', () => {
  it('should find actor by name', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          actors: [
            { actorId: 'actor-001', name: 'TestCube', className: 'Actor' },
          ],
        },
      }),
    };

    const result = await findActorTool.execute(
      { byName: 'TestCube' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.find', {
      byName: 'TestCube',
    });
    expect(result.content[0].text).toContain('TestCube');
  });

  it('should support wildcard search', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          actors: [
            { actorId: 'actor-001', name: 'Cube_1', className: 'Actor' },
            { actorId: 'actor-002', name: 'Cube_2', className: 'Actor' },
          ],
        },
      }),
    };

    const result = await findActorTool.execute(
      { byName: 'Cube*' },
      { connection: mockConnection }
    );

    expect(result.content[0].text).toContain('Found 2 actor(s)');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 actor_find 工具 src/tools/actor/find.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorFindParams } from '../../types/commands.js';

interface ActorInfo {
  actorId: string;
  name: string;
  className: string;
}

export const findActorTool: Tool = {
  name: 'actor_find',
  description: 'Find actors by ID, name, class, or tag. Supports wildcard (*) in name search.',
  inputSchema: {
    type: 'object',
    properties: {
      byId: {
        type: 'string',
        description: 'Find by exact actor ID',
      },
      byName: {
        type: 'string',
        description: 'Find by name (supports wildcard *)',
      },
      byClass: {
        type: 'string',
        description: 'Find by actor class name',
      },
      byTag: {
        type: 'string',
        description: 'Find by actor tag',
      },
    },
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorFindParams;

    const commandParams: Record<string, unknown> = {};
    if (typedParams.byId) commandParams.byId = typedParams.byId;
    if (typedParams.byName) commandParams.byName = typedParams.byName;
    if (typedParams.byClass) commandParams.byClass = typedParams.byClass;
    if (typedParams.byTag) commandParams.byTag = typedParams.byTag;

    if (Object.keys(commandParams).length === 0) {
      return {
        content: [
          {
            type: 'text',
            text: 'Error: At least one search criteria must be provided',
          },
        ],
      };
    }

    const response = await context.connection.sendCommand('actor.find', commandParams);

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to find actors: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { actors: ActorInfo[] };
    const actors = data.actors ?? [];

    if (actors.length === 0) {
      return {
        content: [
          {
            type: 'text',
            text: 'No actors found matching the criteria',
          },
        ],
      };
    }

    const actorList = actors
      .map((a) => `  - ${a.name} (ID: ${a.actorId}, Class: ${a.className})`)
      .join('\n');

    return {
      content: [
        {
          type: 'text',
          text: `Found ${actors.length} actor(s):\n${actorList}`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor_find tool"`

---

### Task 9: Actor工具 - transform (get/set)

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/transform.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/transform.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/transform.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { getTransformTool, setTransformTool } from '../transform.js';

describe('actor_get_transform tool', () => {
  it('should get actor transform', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          location: [100, 200, 300],
          rotation: [0, 90, 0],
          scale: [1, 1, 1],
        },
      }),
    };

    const result = await getTransformTool.execute(
      { actorId: 'actor-001' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.getTransform', {
      actorId: 'actor-001',
    });
    expect(result.content[0].text).toContain('100, 200, 300');
  });
});

describe('actor_set_transform tool', () => {
  it('should set actor transform', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {},
      }),
    };

    const result = await setTransformTool.execute(
      { actorId: 'actor-001', location: [50, 0, 0] },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.setTransform', {
      actorId: 'actor-001',
      location: [50, 0, 0],
    });
    expect(result.content[0].text).toContain('Transform updated');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 transform 工具 src/tools/actor/transform.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorTransformParams, Vector3 } from '../../types/commands.js';

export const getTransformTool: Tool = {
  name: 'actor_get_transform',
  description: 'Get the transform (location, rotation, scale) of an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
    },
    required: ['actorId'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getTransform', {
      actorId: params.actorId,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to get transform: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as {
      location: Vector3;
      rotation: Vector3;
      scale: Vector3;
    };

    return {
      content: [
        {
          type: 'text',
          text: `Transform:\n  Location: [${data.location.join(', ')}]\n  Rotation: [${data.rotation.join(', ')}]\n  Scale: [${data.scale.join(', ')}]`,
        },
      ],
    };
  },
};

export const setTransformTool: Tool = {
  name: 'actor_set_transform',
  description: 'Set the transform (location, rotation, scale) of an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
      location: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'New world position [x, y, z]',
      },
      rotation: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'New rotation in degrees [pitch, yaw, roll]',
      },
      scale: {
        type: 'array',
        items: { type: 'number' },
        minItems: 3,
        maxItems: 3,
        description: 'New scale [x, y, z]',
      },
    },
    required: ['actorId'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorTransformParams;

    const commandParams: Record<string, unknown> = { actorId: typedParams.actorId };
    if (typedParams.location) commandParams.location = typedParams.location;
    if (typedParams.rotation) commandParams.rotation = typedParams.rotation;
    if (typedParams.scale) commandParams.scale = typedParams.scale;

    const response = await context.connection.sendCommand('actor.setTransform', commandParams);

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to set transform: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    return {
      content: [
        {
          type: 'text',
          text: 'Transform updated successfully',
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor transform tools (get/set)"`

---

### Task 10: Actor工具 - property (get/set)

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/property.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/property.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/property.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { getPropertyTool, setPropertyTool } from '../property.js';

describe('actor_get_property tool', () => {
  it('should get actor property', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: { value: true },
      }),
    };

    const result = await getPropertyTool.execute(
      { actorId: 'actor-001', propertyName: 'bHidden' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.getProperty', {
      actorId: 'actor-001',
      propertyName: 'bHidden',
    });
    expect(result.content[0].text).toContain('true');
  });
});

describe('actor_set_property tool', () => {
  it('should set actor property', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {},
      }),
    };

    const result = await setPropertyTool.execute(
      {
        actorId: 'actor-001',
        propertyName: 'bHidden',
        propertyType: 'bool',
        value: true,
      },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.setProperty', {
      actorId: 'actor-001',
      propertyName: 'bHidden',
      propertyType: 'bool',
      value: true,
    });
    expect(result.content[0].text).toContain('Property "bHidden" set');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 property 工具 src/tools/actor/property.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorPropertyParams } from '../../types/commands.js';

export const getPropertyTool: Tool = {
  name: 'actor_get_property',
  description: 'Get a property value from an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
      propertyName: {
        type: 'string',
        description: 'Name of the property to get',
      },
    },
    required: ['actorId', 'propertyName'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getProperty', {
      actorId: params.actorId,
      propertyName: params.propertyName,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to get property: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { value: unknown };
    return {
      content: [
        {
          type: 'text',
          text: `Property "${params.propertyName}": ${JSON.stringify(data.value)}`,
        },
      ],
    };
  },
};

export const setPropertyTool: Tool = {
  name: 'actor_set_property',
  description: 'Set a property value on an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
      propertyName: {
        type: 'string',
        description: 'Name of the property to set',
      },
      propertyType: {
        type: 'string',
        enum: ['int', 'float', 'string', 'bool', 'vector', 'rotator', 'color'],
        description: 'Type of the property value',
      },
      value: {
        description: 'Value to set (type depends on propertyType)',
      },
    },
    required: ['actorId', 'propertyName', 'propertyType', 'value'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorPropertyParams;

    const response = await context.connection.sendCommand('actor.setProperty', {
      actorId: typedParams.actorId,
      propertyName: typedParams.propertyName,
      propertyType: typedParams.propertyType,
      value: typedParams.value,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to set property: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    return {
      content: [
        {
          type: 'text',
          text: `Property "${typedParams.propertyName}" set successfully`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor property tools (get/set)"`

---

### Task 11: Actor工具 - component (add/remove/list)

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/component.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/component.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/component.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { addComponentTool, removeComponentTool, getComponentsTool } from '../component.js';

describe('actor_add_component tool', () => {
  it('should add component to actor', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: { componentId: 'comp-001', name: 'PointLight' },
      }),
    };

    const result = await addComponentTool.execute(
      { actorId: 'actor-001', componentType: 'PointLightComponent', name: 'PointLight' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.addComponent', {
      actorId: 'actor-001',
      componentType: 'PointLightComponent',
      name: 'PointLight',
    });
    expect(result.content[0].text).toContain('comp-001');
  });
});

describe('actor_remove_component tool', () => {
  it('should remove component from actor', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {},
      }),
    };

    const result = await removeComponentTool.execute(
      { actorId: 'actor-001', componentId: 'comp-001' },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.removeComponent', {
      actorId: 'actor-001',
      componentId: 'comp-001',
    });
    expect(result.content[0].text).toContain('removed');
  });
});

describe('actor_get_components tool', () => {
  it('should list actor components', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          components: [
            { componentId: 'comp-001', name: 'StaticMesh', className: 'StaticMeshComponent' },
            { componentId: 'comp-002', name: 'PointLight', className: 'PointLightComponent' },
          ],
        },
      }),
    };

    const result = await getComponentsTool.execute(
      { actorId: 'actor-001' },
      { connection: mockConnection }
    );

    expect(result.content[0].text).toContain('2 component(s)');
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 component 工具 src/tools/actor/component.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';
import { ActorComponentParams, ActorRemoveComponentParams } from '../../types/commands.js';

interface ComponentInfo {
  componentId: string;
  name: string;
  className: string;
}

export const addComponentTool: Tool = {
  name: 'actor_add_component',
  description: 'Add a component to an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
      componentType: {
        type: 'string',
        description: 'Type of component to add (e.g., "StaticMeshComponent", "PointLightComponent")',
      },
      name: {
        type: 'string',
        description: 'Name for the component (optional)',
      },
    },
    required: ['actorId', 'componentType'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorComponentParams;

    const response = await context.connection.sendCommand('actor.addComponent', {
      actorId: typedParams.actorId,
      componentType: typedParams.componentType,
      name: typedParams.name,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to add component: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { componentId: string; name: string };
    return {
      content: [
        {
          type: 'text',
          text: `Added component "${data.name}" with ID: ${data.componentId}`,
        },
      ],
    };
  },
};

export const removeComponentTool: Tool = {
  name: 'actor_remove_component',
  description: 'Remove a component from an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
      componentId: {
        type: 'string',
        description: 'ID of the component to remove',
      },
    },
    required: ['actorId', 'componentId'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const typedParams = params as ActorRemoveComponentParams;

    const response = await context.connection.sendCommand('actor.removeComponent', {
      actorId: typedParams.actorId,
      componentId: typedParams.componentId,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to remove component: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    return {
      content: [
        {
          type: 'text',
          text: `Component "${typedParams.componentId}" removed successfully`,
        },
      ],
    };
  },
};

export const getComponentsTool: Tool = {
  name: 'actor_get_components',
  description: 'List all components attached to an actor.',
  inputSchema: {
    type: 'object',
    properties: {
      actorId: {
        type: 'string',
        description: 'ID of the actor',
      },
    },
    required: ['actorId'],
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.getComponents', {
      actorId: params.actorId,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to get components: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { components: ComponentInfo[] };
    const components = data.components ?? [];

    if (components.length === 0) {
      return {
        content: [
          {
            type: 'text',
            text: 'No components found on this actor',
          },
        ],
      };
    }

    const compList = components
      .map((c) => `  - ${c.name} (${c.className}, ID: ${c.componentId})`)
      .join('\n');

    return {
      content: [
        {
          type: 'text',
          text: `${components.length} component(s):\n${compList}`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor component tools (add/remove/list)"`

---

### Task 12: Actor工具 - list

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/list.ts`
- Create: `D:/unreal-mcp-server/src/tools/actor/__tests__/list.test.ts`

- [ ] **Step 1: 创建测试文件 src/tools/actor/__tests__/list.test.ts**

```typescript
import { describe, it, expect, vi } from 'vitest';
import { listActorsTool } from '../list.js';

describe('actor_list tool', () => {
  it('should list all actors', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          actors: [
            { actorId: 'actor-001', name: 'Cube', className: 'StaticMeshActor' },
            { actorId: 'actor-002', name: 'Light', className: 'PointLight' },
          ],
        },
      }),
    };

    const result = await listActorsTool.execute({}, { connection: mockConnection });

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.list', {});
    expect(result.content[0].text).toContain('2 actor(s)');
  });

  it('should filter by class', async () => {
    const mockConnection = {
      sendCommand: vi.fn().mockResolvedValue({
        id: '123',
        success: true,
        data: {
          actors: [
            { actorId: 'actor-001', name: 'Light1', className: 'PointLight' },
          ],
        },
      }),
    };

    await listActorsTool.execute(
      { filter: { class: 'PointLight' } },
      { connection: mockConnection }
    );

    expect(mockConnection.sendCommand).toHaveBeenCalledWith('actor.list', {
      filter: { class: 'PointLight' },
    });
  });
});
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: FAIL

- [ ] **Step 3: 实现 actor_list 工具 src/tools/actor/list.ts**

```typescript
import { Tool, ToolContext, ToolResult } from '../registry.js';

interface ActorInfo {
  actorId: string;
  name: string;
  className: string;
}

export const listActorsTool: Tool = {
  name: 'actor_list',
  description: 'List all actors in the current level. Optionally filter by class.',
  inputSchema: {
    type: 'object',
    properties: {
      filter: {
        type: 'object',
        properties: {
          class: {
            type: 'string',
            description: 'Filter by actor class name',
          },
        },
      },
    },
  },

  async execute(
    params: Record<string, unknown>,
    context: ToolContext
  ): Promise<ToolResult> {
    const response = await context.connection.sendCommand('actor.list', {
      filter: params.filter,
    });

    if (!response.success) {
      return {
        content: [
          {
            type: 'text',
            text: `Failed to list actors: ${response.error?.message ?? 'Unknown error'}`,
          },
        ],
      };
    }

    const data = response.data as { actors: ActorInfo[] };
    const actors = data.actors ?? [];

    if (actors.length === 0) {
      return {
        content: [
          {
            type: 'text',
            text: 'No actors found in the level',
          },
        ],
      };
    }

    const actorList = actors
      .map((a) => `  - ${a.name} (${a.className}, ID: ${a.actorId})`)
      .join('\n');

    return {
      content: [
        {
          type: 'text',
          text: `${actors.length} actor(s) in level:\n${actorList}`,
        },
      ],
    };
  },
};
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cd /d/unreal-mcp-server && npm test`
Expected: PASS

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement actor_list tool"`

---

### Task 13: Actor工具模块导出

**Files:**
- Create: `D:/unreal-mcp-server/src/tools/actor/index.ts`

- [ ] **Step 1: 创建模块索引 src/tools/actor/index.ts**

```typescript
import { Tool } from '../registry.js';
import { createActorTool } from './create.js';
import { deleteActorTool } from './delete.js';
import { findActorTool } from './find.js';
import { listActorsTool } from './list.js';
import { getTransformTool, setTransformTool } from './transform.js';
import { getPropertyTool, setPropertyTool } from './property.js';
import { addComponentTool, removeComponentTool, getComponentsTool } from './component.js';

export const actorTools: Tool[] = [
  createActorTool,
  deleteActorTool,
  findActorTool,
  listActorsTool,
  getTransformTool,
  setTransformTool,
  getPropertyTool,
  setPropertyTool,
  addComponentTool,
  removeComponentTool,
  getComponentsTool,
];

export * from './create.js';
export * from './delete.js';
export * from './find.js';
export * from './list.js';
export * from './transform.js';
export * from './property.js';
export * from './component.js';
```

- [ ] **Step 2: 验证编译**

Run: `cd /d/unreal-mcp-server && npm run build`
Expected: 编译成功

- [ ] **Step 3: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: export actor tools module"`

---

## Phase 3: MCP服务器集成

### Task 14: MCP协议处理

**Files:**
- Create: `D:/unreal-mcp-server/src/core/protocol.ts`
- Modify: `D:/unreal-mcp-server/src/index.ts`

- [ ] **Step 1: 创建 MCP 协议处理 src/core/protocol.ts**

```typescript
import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
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
  server.setRequestHandler(ListToolsRequestSchema, async () => {
    const tools = registry.getAll().map((tool) => ({
      name: tool.name,
      description: tool.description,
      inputSchema: tool.inputSchema,
    }));

    return { tools };
  });

  // Call tool handler
  server.setRequestHandler(CallToolRequestSchema, async (request) => {
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

      return await tool.execute(args ?? {}, context);
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
```

- [ ] **Step 2: 更新入口文件 src/index.ts**

```typescript
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
    port: parseInt(process.env.UNREAL_PORT ?? '8080', 10),
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
  console.error(`Connecting to Unreal at ${process.env.UNREAL_HOST ?? 'localhost'}:${process.env.UNREAL_PORT ?? '8080'}`);
}

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});
```

- [ ] **Step 3: 验证编译**

Run: `cd /d/unreal-mcp-server && npm run build`
Expected: 编译成功

- [ ] **Step 4: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: integrate MCP protocol layer and update entry point"`

---

## Phase 4: Unreal插件基础

### Task 15: Unreal插件结构

**Files:**
- Create: `D:/unreal-mcp-server/unreal-plugin/UnrealMCP.uplugin`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP.Build.cs`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Public/UnrealMCP.h`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Private/UnrealMCP.cpp`

- [ ] **Step 1: 创建插件清单 unreal-plugin/UnrealMCP.uplugin**

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "Unreal MCP",
    "Description": "MCP server integration for Unreal Engine",
    "Category": "Editor",
    "CreatedBy": "Unreal MCP Team",
    "CreatedByURL": "",
    "DocsURL": "",
    "MarketplaceURL": "",
    "SupportURL": "",
    "CanContainContent": false,
    "IsBetaVersion": false,
    "IsExperimentalVersion": false,
    "Installed": false,
    "EngineVersion": "5.6.0",
    "EngineVersionMax": "5.7.*",
    "Modules": [
        {
            "Name": "UnrealMCP",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "WebSockets",
            "Enabled": true
        },
        {
            "Name": "JsonUtilities",
            "Enabled": true
        }
    ]
}
```

- [ ] **Step 2: 创建构建脚本 unreal-plugin/Source/UnrealMCP.Build.cs**

```csharp
using UnrealBuildTool;

public class UnrealMCP : ModuleRules
{
    public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Json",
            "JsonUtilities",
            "WebSockets",
            "Networking"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore"
        });
    }
}
```

- [ ] **Step 3: 创建模块头文件 unreal-plugin/Source/UnrealMCP/Public/UnrealMCP.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUnrealMCPModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    bool bInitialized = false;
};
```

- [ ] **Step 4: 创建模块实现 unreal-plugin/Source/UnrealMCP/Private/UnrealMCP.cpp**

```cpp
#include "UnrealMCP.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

void FUnrealMCPModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module starting up..."));
    bInitialized = true;
}

void FUnrealMCPModule::ShutdownModule()
{
    if (!bInitialized)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module shutting down..."));
    bInitialized = false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
```

- [ ] **Step 5: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: create Unreal plugin base structure"`

---

### Task 16: WebSocket服务器

**Files:**
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Public/WebSocketServer.h`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Private/WebSocketServer.cpp`

- [ ] **Step 1: 创建 WebSocket 服务器头文件 unreal-plugin/Source/UnrealMCP/Public/WebSocketServer.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "WebSocketsModule.h"
#include "IWebSocketServer.h"
#include "WebSocketServer.generated.h"

DECLARE_DELEGATE_OneParam(FOnCommandReceived, const TSharedPtr<FJsonObject>&);
DECLARE_DELEGATE_OneParam(FOnClientConnected, const FString&);

UCLASS()
class UNREALMCP_API UWebSocketServer : public UObject
{
    GENERATED_BODY()

public:
    UWebSocketServer();

    bool Start(int32 Port = 8080);
    void Stop();

    void SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data);
    void SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data);

    FOnCommandReceived OnCommandReceived;
    FOnClientConnected OnClientConnected;

    bool IsRunning() const { return bIsRunning; }

private:
    TSharedPtr<IWebSocketServer> Server;
    TSharedPtr<IWebSocket> ConnectedClient;
    bool bIsRunning = false;
    int32 ServerPort = 8080;

    void HandleMessage(const FString& Message);
    FString GenerateUUID();
};
```

- [ ] **Step 2: 创建 WebSocket 服务器实现 unreal-plugin/Source/UnrealMCP/Private/WebSocketServer.cpp**

```cpp
#include "WebSocketServer.h"
#include "JsonObjectConverter.h"
#include "Misc/Guid.h"

UWebSocketServer::UWebSocketServer()
{
}

bool UWebSocketServer::Start(int32 Port)
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("WebSocketServer already running"));
        return true;
    }

    ServerPort = Port;

    FWebSocketServerConfig Config;
    Config.Port = Port;

    Server = IWebSocketServer::Create(Config);

    if (!Server.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create WebSocket server"));
        return false;
    }

    Server->OnClientConnected().BindLambda([this](TSharedPtr<IWebSocket> Client)
    {
        ConnectedClient = Client;

        Client->OnMessage().BindLambda([this](const FString& Message)
        {
            HandleMessage(Message);
        });

        Client->OnClosed().BindLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
        {
            UE_LOG(LogTemp, Log, TEXT("Client disconnected: %s"), *Reason);
            ConnectedClient.Reset();
        });

        UE_LOG(LogTemp, Log, TEXT("Client connected"));
        OnClientConnected.ExecuteIfBound(Client->GetRemoteEndpoint());
    });

    if (!Server->Start())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server on port %d"), Port);
        return false;
    }

    bIsRunning = true;
    UE_LOG(LogTemp, Log, TEXT("WebSocketServer started on port %d"), Port);
    return true;
}

void UWebSocketServer::Stop()
{
    if (!bIsRunning)
    {
        return;
    }

    if (ConnectedClient.IsValid())
    {
        ConnectedClient->Close();
        ConnectedClient.Reset();
    }

    if (Server.IsValid())
    {
        Server->Stop();
        Server.Reset();
    }

    bIsRunning = false;
    UE_LOG(LogTemp, Log, TEXT("WebSocketServer stopped"));
}

void UWebSocketServer::SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("No client connected"));
        return;
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetStringField("id", Id);
    Response->SetBoolField("success", bSuccess);

    if (bSuccess && Data.IsValid())
    {
        Response->SetObjectField("data", Data);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    ConnectedClient->Send(JsonString);
}

void UWebSocketServer::SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid())
    {
        return;
    }

    TSharedPtr<FJsonObject> Event = MakeShareable(new FJsonObject);
    Event->SetStringField("event", EventType);
    Event->SetStringField("timestamp", FDateTime::UtcNow().ToIso8601());

    if (Data.IsValid())
    {
        Event->SetObjectField("data", Data);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Event.ToSharedRef(), Writer);

    ConnectedClient->Send(JsonString);
}

void UWebSocketServer::HandleMessage(const FString& Message)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON message: %s"), *Message);
        return;
    }

    OnCommandReceived.ExecuteIfBound(JsonObject);
}

FString UWebSocketServer::GenerateUUID()
{
    return FGuid::NewGuid().ToString(EGuidFormats::DigitsLower);
}
```

- [ ] **Step 3: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement WebSocket server for Unreal plugin"`

---

### Task 17: 命令分发器

**Files:**
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Public/Handlers/IHandler.h`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Public/CommandDispatcher.h`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Private/CommandDispatcher.cpp`

- [ ] **Step 1: 创建 Handler 接口 unreal-plugin/Source/UnrealMCP/Public/Handlers/IHandler.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class IHandler
{
public:
    virtual ~IHandler() = default;

    virtual FString GetPrefix() const = 0;
    virtual TSharedPtr<FJsonObject> Handle(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError) = 0;
};
```

- [ ] **Step 2: 创建命令分发器头文件 unreal-plugin/Source/UnrealMCP/Public/CommandDispatcher.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Handlers/IHandler.h"
#include "CommandDispatcher.generated.h"

UCLASS()
class UNREALMCP_API UCommandDispatcher : public UObject
{
    GENERATED_BODY()

public:
    UCommandDispatcher();

    void RegisterHandler(TSharedPtr<IHandler> Handler);
    TSharedPtr<FJsonObject> Dispatch(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError);

private:
    TMap<FString, TSharedPtr<IHandler>> Handlers;
};
```

- [ ] **Step 3: 创建命令分发器实现 unreal-plugin/Source/UnrealMCP/Private/CommandDispatcher.cpp**

```cpp
#include "CommandDispatcher.h"

UCommandDispatcher::UCommandDispatcher()
{
}

void UCommandDispatcher::RegisterHandler(TSharedPtr<IHandler> Handler)
{
    if (Handler.IsValid())
    {
        Handlers.Add(Handler->GetPrefix(), Handler);
    }
}

TSharedPtr<FJsonObject> UCommandDispatcher::Dispatch(
    const FString& Command,
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    // Parse command prefix (e.g., "actor.create" -> prefix "actor", subcommand "create")
    int32 DotIndex;
    if (!Command.FindChar('.', DotIndex))
    {
        OutError = FString::Printf(TEXT("Invalid command format: %s"), *Command);
        return nullptr;
    }

    FString Prefix = Command.Left(DotIndex);
    FString SubCommand = Command.RightChop(DotIndex + 1);

    TSharedPtr<IHandler>* HandlerPtr = Handlers.Find(Prefix);
    if (!HandlerPtr || !HandlerPtr->IsValid())
    {
        OutError = FString::Printf(TEXT("No handler found for prefix: %s"), *Prefix);
        return nullptr;
    }

    return (*HandlerPtr)->Handle(SubCommand, Params, OutError);
}
```

- [ ] **Step 4: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement command dispatcher with handler interface"`

---

### Task 18: Actor Handler

**Files:**
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Public/Handlers/ActorHandler.h`
- Create: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Private/Handlers/ActorHandler.cpp`

- [ ] **Step 1: 创建 Actor Handler 头文件 unreal-plugin/Source/UnrealMCP/Public/Handlers/ActorHandler.h**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "IHandler.h"
#include "ActorHandler.generated.h"

UCLASS()
class UNREALMCP_API UActorHandler : public UObject, public IHandler
{
    GENERATED_BODY()

public:
    virtual FString GetPrefix() const override { return TEXT("actor"); }
    virtual TSharedPtr<FJsonObject> Handle(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError) override;

private:
    TSharedPtr<FJsonObject> HandleCreate(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleDelete(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleFind(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleList(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleSetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleSetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleAddComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleRemoveComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetComponents(const TSharedPtr<FJsonObject>& Params, FString& OutError);

    AActor* FindActorById(const FString& ActorId);
    AActor* FindActorByName(const FString& Name);
    FString GetActorId(AActor* Actor);
};
```

- [ ] **Step 2: 创建 Actor Handler 实现 unreal-plugin/Source/UnrealMCP/Private/Handlers/ActorHandler.cpp**

```cpp
#include "ActorHandler.h"
#include "Engine/World.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "JsonObjectConverter.h"

TSharedPtr<FJsonObject> UActorHandler::Handle(
    const FString& Command,
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    if (Command == TEXT("create")) return HandleCreate(Params, OutError);
    if (Command == TEXT("delete")) return HandleDelete(Params, OutError);
    if (Command == TEXT("find")) return HandleFind(Params, OutError);
    if (Command == TEXT("list")) return HandleList(Params, OutError);
    if (Command == TEXT("getTransform")) return HandleGetTransform(Params, OutError);
    if (Command == TEXT("setTransform")) return HandleSetTransform(Params, OutError);
    if (Command == TEXT("getProperty")) return HandleGetProperty(Params, OutError);
    if (Command == TEXT("setProperty")) return HandleSetProperty(Params, OutError);
    if (Command == TEXT("addComponent")) return HandleAddComponent(Params, OutError);
    if (Command == TEXT("removeComponent")) return HandleRemoveComponent(Params, OutError);
    if (Command == TEXT("getComponents")) return HandleGetComponents(Params, OutError);

    OutError = FString::Printf(TEXT("Unknown actor command: %s"), *Command);
    return nullptr;
}

TSharedPtr<FJsonObject> UActorHandler::HandleCreate(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString Name;
    if (!Params->TryGetStringField("name", Name))
    {
        OutError = TEXT("Missing required field: name");
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> LocationArray;
    FVector Location = FVector::ZeroVector;
    if (Params->TryGetArrayField("location", LocationArray) && LocationArray.Num() == 3)
    {
        Location = FVector(
            LocationArray[0]->AsNumber(),
            LocationArray[1]->AsNumber(),
            LocationArray[2]->AsNumber()
        );
    }

    FString ActorClass = TEXT("Actor");
    Params->TryGetStringField("actorClass", ActorClass);

    UWorld* World = GWorld;
    if (!World)
    {
        OutError = TEXT("No world available");
        return nullptr;
    }

    // Find actor class by name
    UClass* ActorClassPtr = nullptr;
    for (TObjectIterator<UClass> It; It; ++It)
    {
        if (It->IsChildOf(AActor::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
        {
            if (It->GetName().Contains(ActorClass))
            {
                ActorClassPtr = *It;
                break;
            }
        }
    }

    if (!ActorClassPtr)
    {
        ActorClassPtr = AActor::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(*Name);

    AActor* NewActor = World->SpawnActor<AActor>(ActorClassPtr, Location, FRotator::ZeroRotator, SpawnParams);

    if (!NewActor)
    {
        OutError = FString::Printf(TEXT("Failed to spawn actor: %s"), *Name);
        return nullptr;
    }

    NewActor->SetActorLabel(Name);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("actorId", GetActorId(NewActor));
    Result->SetStringField("name", Name);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleDelete(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId;
    FString Name;

    AActor* Actor = nullptr;

    if (Params->TryGetStringField("actorId", ActorId))
    {
        Actor = FindActorById(ActorId);
    }
    else if (Params->TryGetStringField("name", Name))
    {
        Actor = FindActorByName(Name);
    }
    else
    {
        OutError = TEXT("Missing required field: actorId or name");
        return nullptr;
    }

    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    Actor->Destroy();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField("deleted", true);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleFind(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    UWorld* World = GWorld;
    if (!World)
    {
        OutError = TEXT("No world available");
        return nullptr;
    }

    TArray<AActor*> FoundActors;

    FString ById, ByName, ByClass, ByTag;
    bool bHasFilter = false;

    if (Params->TryGetStringField("byId", ById))
    {
        bHasFilter = true;
        if (AActor* Actor = FindActorById(ById))
        {
            FoundActors.Add(Actor);
        }
    }

    if (Params->TryGetStringField("byName", ByName))
    {
        bHasFilter = true;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetActorLabel().Contains(ByName.Replace(TEXT("*"), TEXT(""))))
            {
                FoundActors.Add(*It);
            }
        }
    }

    if (Params->TryGetStringField("byClass", ByClass))
    {
        bHasFilter = true;
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetClass()->GetName().Contains(ByClass))
            {
                FoundActors.Add(*It);
            }
        }
    }

    if (!bHasFilter)
    {
        OutError = TEXT("At least one search criteria required");
        return nullptr;
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> ActorsArray;

    for (AActor* Actor : FoundActors)
    {
        TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
        ActorObj->SetStringField("actorId", GetActorId(Actor));
        ActorObj->SetStringField("name", Actor->GetActorLabel());
        ActorObj->SetStringField("className", Actor->GetClass()->GetName());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
    }

    Result->SetArrayField("actors", ActorsArray);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleList(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    UWorld* World = GWorld;
    if (!World)
    {
        OutError = TEXT("No world available");
        return nullptr;
    }

    FString ClassFilter;
    const TSharedPtr<FJsonObject>* FilterObj;
    if (Params->TryGetObjectField("filter", FilterObj))
    {
        (*FilterObj)->TryGetStringField("class", ClassFilter);
    }

    TArray<TSharedPtr<FJsonValue>> ActorsArray;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (!ClassFilter.IsEmpty() && !It->GetClass()->GetName().Contains(ClassFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
        ActorObj->SetStringField("actorId", GetActorId(*It));
        ActorObj->SetStringField("name", It->GetActorLabel());
        ActorObj->SetStringField("className", It->GetClass()->GetName());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField("actors", ActorsArray);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetTransform(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    FTransform Transform = Actor->GetActorTransform();
    FVector Location = Transform.GetTranslation();
    FRotator Rotation = Transform.GetRotation().Rotator();
    FVector Scale = Transform.GetScale3D();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);

    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    Result->SetArrayField("location", LocationArray);

    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Pitch)));
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Yaw)));
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Roll)));
    Result->SetArrayField("rotation", RotationArray);

    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.X)));
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Y)));
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Z)));
    Result->SetArrayField("scale", ScaleArray);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetTransform(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> LocationArray;
    if (Params->TryGetArrayField("location", LocationArray) && LocationArray.Num() == 3)
    {
        FVector Location(
            LocationArray[0]->AsNumber(),
            LocationArray[1]->AsNumber(),
            LocationArray[2]->AsNumber()
        );
        Actor->SetActorLocation(Location);
    }

    TArray<TSharedPtr<FJsonValue>> RotationArray;
    if (Params->TryGetArrayField("rotation", RotationArray) && RotationArray.Num() == 3)
    {
        FRotator Rotation(
            RotationArray[0]->AsNumber(),
            RotationArray[1]->AsNumber(),
            RotationArray[2]->AsNumber()
        );
        Actor->SetActorRotation(Rotation);
    }

    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    if (Params->TryGetArrayField("scale", ScaleArray) && ScaleArray.Num() == 3)
    {
        FVector Scale(
            ScaleArray[0]->AsNumber(),
            ScaleArray[1]->AsNumber(),
            ScaleArray[2]->AsNumber()
        );
        Actor->SetActorScale3D(Scale);
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField("updated", true);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetProperty(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId, PropertyName;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }
    if (!Params->TryGetStringField("propertyName", PropertyName))
    {
        OutError = TEXT("Missing required field: propertyName");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    // Get property value using reflection
    FProperty* Property = Actor->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);

    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        Result->SetBoolField("value", BoolProp->GetPropertyValue_InContainer(Actor));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        Result->SetNumberField("value", IntProp->GetPropertyValue_InContainer(Actor));
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        Result->SetNumberField("value", FloatProp->GetPropertyValue_InContainer(Actor));
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        Result->SetStringField("value", StrProp->GetPropertyValue_InContainer(Actor));
    }
    else
    {
        OutError = FString::Printf(TEXT("Unsupported property type: %s"), *Property->GetClass()->GetName());
        return nullptr;
    }

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetProperty(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId, PropertyName, PropertyType;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }
    if (!Params->TryGetStringField("propertyName", PropertyName))
    {
        OutError = TEXT("Missing required field: propertyName");
        return nullptr;
    }
    if (!Params->TryGetStringField("propertyType", PropertyType))
    {
        OutError = TEXT("Missing required field: propertyType");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    FProperty* Property = Actor->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    // Set property value based on type
    if (PropertyType == TEXT("bool"))
    {
        if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
        {
            bool Value;
            if (Params->TryGetBoolField("value", Value))
            {
                BoolProp->SetPropertyValue_InContainer(Actor, Value);
            }
        }
    }
    else if (PropertyType == TEXT("int"))
    {
        if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
        {
            int32 Value;
            if (Params->TryGetNumberField("value", Value))
            {
                IntProp->SetPropertyValue_InContainer(Actor, Value);
            }
        }
    }
    else if (PropertyType == TEXT("float"))
    {
        if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
        {
            double Value;
            if (Params->TryGetNumberField("value", Value))
            {
                FloatProp->SetPropertyValue_InContainer(Actor, Value);
            }
        }
    }
    else if (PropertyType == TEXT("string"))
    {
        if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
        {
            FString Value;
            if (Params->TryGetStringField("value", Value))
            {
                StrProp->SetPropertyValue_InContainer(Actor, Value);
            }
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField("set", true);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleAddComponent(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId, ComponentType;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }
    if (!Params->TryGetStringField("componentType", ComponentType))
    {
        OutError = TEXT("Missing required field: componentType");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    // Find component class
    UClass* ComponentClass = nullptr;
    for (TObjectIterator<UClass> It; It; ++It)
    {
        if (It->IsChildOf(UActorComponent::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
        {
            if (It->GetName().Contains(ComponentType))
            {
                ComponentClass = *It;
                break;
            }
        }
    }

    if (!ComponentClass)
    {
        OutError = FString::Printf(TEXT("Component class not found: %s"), *ComponentType);
        return nullptr;
    }

    FString ComponentName;
    Params->TryGetStringField("name", ComponentName);
    if (ComponentName.IsEmpty())
    {
        ComponentName = ComponentType;
    }

    UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, ComponentClass, FName(*ComponentName));
    if (!NewComponent)
    {
        OutError = TEXT("Failed to create component");
        return nullptr;
    }

    NewComponent->RegisterComponent();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("componentId", NewComponent->GetName());
    Result->SetStringField("name", ComponentName);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleRemoveComponent(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId, ComponentId;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }
    if (!Params->TryGetStringField("componentId", ComponentId))
    {
        OutError = TEXT("Missing required field: componentId");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    UActorComponent* Component = Actor->FindComponentByClass<UActorComponent>();
    if (!Component || Component->GetName() != ComponentId)
    {
        OutError = TEXT("Component not found");
        return nullptr;
    }

    Component->DestroyComponent();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetBoolField("removed", true);
    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetComponents(
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField("actorId", ActorId))
    {
        OutError = TEXT("Missing required field: actorId");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject);
        CompObj->SetStringField("componentId", Component->GetName());
        CompObj->SetStringField("name", Component->GetName());
        CompObj->SetStringField("className", Component->GetClass()->GetName());
        ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField("components", ComponentsArray);
    return Result;
}

AActor* UActorHandler::FindActorById(const FString& ActorId)
{
    UWorld* World = GWorld;
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (GetActorId(*It) == ActorId)
        {
            return *It;
        }
    }
    return nullptr;
}

AActor* UActorHandler::FindActorByName(const FString& Name)
{
    UWorld* World = GWorld;
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetActorLabel() == Name)
        {
            return *It;
        }
    }
    return nullptr;
}

FString UActorHandler::GetActorId(AActor* Actor)
{
    if (!Actor) return TEXT("");
    return FString::Printf(TEXT("%s_%d"), *Actor->GetActorLabel(), Actor->GetUniqueID());
}
```

- [ ] **Step 3: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: implement ActorHandler with all actor commands"`

---

### Task 19: 集成插件组件

**Files:**
- Modify: `D:/unreal-mcp-server/unreal-plugin/Source/UnrealMCP/Private/UnrealMCP.cpp`

- [ ] **Step 1: 更新模块实现以集成所有组件**

```cpp
#include "UnrealMCP.h"
#include "WebSocketServer.h"
#include "CommandDispatcher.h"
#include "Handlers/ActorHandler.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

static UWebSocketServer* GWebSocketServer = nullptr;
static UCommandDispatcher* GCommandDispatcher = nullptr;

void FUnrealMCPModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module starting up..."));

    // Create command dispatcher
    GCommandDispatcher = NewObject<UCommandDispatcher>();
    GCommandDispatcher->AddToRoot();

    // Register handlers
    TSharedPtr<IHandler> ActorHandler = MakeShareable(new UActorHandler());
    GCommandDispatcher->RegisterHandler(ActorHandler);

    // Create and start WebSocket server
    GWebSocketServer = NewObject<UWebSocketServer>();
    GWebSocketServer->AddToRoot();

    GWebSocketServer->OnCommandReceived.BindLambda([](const TSharedPtr<FJsonObject>& Command)
    {
        FString Id;
        FString Cmd;
        Command->TryGetStringField("id", Id);
        Command->TryGetStringField("cmd", Cmd);

        const TSharedPtr<FJsonObject>* Params;
        Command->TryGetObjectField("params", Params);

        FString Error;
        TSharedPtr<FJsonObject> Result = GCommandDispatcher->Dispatch(Cmd, Params ? *Params : MakeShareable(new FJsonObject), Error);

        if (Result.IsValid())
        {
            GWebSocketServer->SendResponse(Id, true, Result);
        }
        else
        {
            TSharedPtr<FJsonObject> ErrorObj = MakeShareable(new FJsonObject);
            ErrorObj->SetStringField("code", "COMMAND_FAILED");
            ErrorObj->SetStringField("type", "runtime_error");
            ErrorObj->SetStringField("message", Error);
            GWebSocketServer->SendResponse(Id, false, ErrorObj);
        }
    });

    if (GWebSocketServer->Start(8080))
    {
        UE_LOG(LogTemp, Log, TEXT("WebSocket server started on port 8080"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server"));
    }

    bInitialized = true;
}

void FUnrealMCPModule::ShutdownModule()
{
    if (!bInitialized)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module shutting down..."));

    if (GWebSocketServer)
    {
        GWebSocketServer->Stop();
        GWebSocketServer->RemoveFromRoot();
        GWebSocketServer = nullptr;
    }

    if (GCommandDispatcher)
    {
        GCommandDispatcher->RemoveFromRoot();
        GCommandDispatcher = nullptr;
    }

    bInitialized = false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
```

- [ ] **Step 2: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "feat: integrate all plugin components in module startup"`

---

### Task 20: 创建README

**Files:**
- Create: `D:/unreal-mcp-server/README.md`

- [ ] **Step 1: 创建 README.md**

```markdown
# Unreal MCP Server

MCP服务器，用于通过Claude操作Unreal Engine编辑器。

## 架构

```
Claude → MCP Server (TypeScript) → WebSocket → Unreal Plugin (C++)
```

## 快速开始

### 1. 安装MCP服务器

```bash
cd unreal-mcp-server
npm install
npm run build
```

### 2. 安装Unreal插件

1. 将 `unreal-plugin` 目录复制到你的Unreal项目的 `Plugins` 目录
2. 在Unreal编辑器中启用 "Unreal MCP" 插件
3. 重启Unreal编辑器

### 3. 配置MCP

在Claude Code中添加MCP服务器配置：

```json
{
  "mcpServers": {
    "unreal": {
      "command": "node",
      "args": ["D:/unreal-mcp-server/dist/index.js"],
      "env": {
        "UNREAL_HOST": "localhost",
        "UNREAL_PORT": "8080"
      }
    }
  }
}
```

## 可用工具

### Actor操作

| 工具 | 描述 |
|------|------|
| `actor_create` | 创建Actor |
| `actor_delete` | 删除Actor |
| `actor_find` | 查找Actor |
| `actor_list` | 列出所有Actor |
| `actor_get_transform` | 获取Transform |
| `actor_set_transform` | 设置Transform |
| `actor_get_property` | 获取属性 |
| `actor_set_property` | 设置属性 |
| `actor_add_component` | 添加组件 |
| `actor_remove_component` | 移除组件 |
| `actor_get_components` | 获取组件列表 |

## 示例用法

```
用户: 创建一个叫 "MyCube" 的Actor
Claude: [调用 actor_create 工具]
        已创建 Actor "MyCube"，ID: MyCube_123

用户: 把它移动到 (100, 200, 300)
Claude: [调用 actor_set_transform 工具]
        Transform 已更新

用户: 给它添加一个 PointLightComponent
Claude: [调用 actor_add_component 工具]
        已添加组件 PointLight
```

## 扩展

要添加新的工具模块，请参考 `docs/superpowers/specs/2026-04-27-unreal-mcp-design.md`。

## 版本支持

- Unreal Engine 5.6
- Unreal Engine 5.7
```

- [ ] **Step 2: 提交**

Run: `cd /d/unreal-mcp-server && git add . && git commit -m "docs: add README with usage instructions"`

---

## 实施检查清单

- [ ] Phase 1 完成：MCP Server 基础设施
  - [ ] Task 1: 项目初始化
  - [ ] Task 2: 类型定义
  - [ ] Task 3: 连接管理器
  - [ ] Task 4: 事件分发器
  - [ ] Task 5: 工具注册中心

- [ ] Phase 2 完成：Actor工具集
  - [ ] Task 6: actor_create
  - [ ] Task 7: actor_delete
  - [ ] Task 8: actor_find
  - [ ] Task 9: actor_transform
  - [ ] Task 10: actor_property
  - [ ] Task 11: actor_component
  - [ ] Task 12: actor_list
  - [ ] Task 13: 模块导出

- [ ] Phase 3 完成：MCP服务器集成
  - [ ] Task 14: MCP协议处理

- [ ] Phase 4 完成：Unreal插件
  - [ ] Task 15: 插件结构
  - [ ] Task 16: WebSocket服务器
  - [ ] Task 17: 命令分发器
  - [ ] Task 18: ActorHandler
  - [ ] Task 19: 集成组件
  - [ ] Task 20: README
