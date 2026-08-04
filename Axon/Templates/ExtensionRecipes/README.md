# Axon ExtensionRecipes

Drop optional recipe JSON files here (`*.json`, not starting with `_`).

They are listed by `axon_list_extension_recipes` and can be referenced via
`recipe_id` on `axon_create_extension` when you choose to maintain shared
scaffolds outside of chat.

**Knowledge packs are different:** use `knowledge_query` → `scaffold_kb_plugin`
(see `project_kb_plugin.json` as a documentation pointer only). Do not use
`axon_create_extension` to build `AxonXxxKB` plugins.

## Schema (informational)

```json
{
  "plugin_name": "AxonFoo",
  "namespace": "foo",
  "intent": "optional blurb",
  "depend_plugins": ["SomePlugin"],
  "build_modules": ["SomeModule"],
  "actions": [
    { "name": "inspect", "description": "What it does", "params": {} }
  ]
}
```

Recommendation workflow is NOT automated by recipes: agents must still ask the
user whether to recommend actions, research the target system first, then
confirm the action list before scaffolding.
