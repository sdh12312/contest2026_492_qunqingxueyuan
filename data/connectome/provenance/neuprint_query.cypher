MATCH (pre:Neuron)-[e:ConnectsTo]->(post:Neuron)
WHERE pre.type IN ["LPLC2", "LC4"] AND post.type = "DNp01"
RETURN pre.bodyId AS pre_body_id,
       pre.instance AS pre_instance,
       pre.type AS pre_cell_type,
       post.bodyId AS post_body_id,
       post.instance AS post_instance,
       post.type AS post_cell_type,
       e.weight AS synapse_count
ORDER BY pre.type, post.bodyId, e.weight DESC

