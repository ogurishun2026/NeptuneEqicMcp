export interface Tool {
  name: string;
  description: string;
  inputSchema: {
    type: 'object';
    properties?: { [key: string]: object };
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
