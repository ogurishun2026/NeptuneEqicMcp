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
      execute: async () => ({ content: [{ type: 'text', text: 'ok' }] }),
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
      execute: async () => ({ content: [{ type: 'text', text: 'ok' }] }),
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
