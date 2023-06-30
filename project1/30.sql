SELECT id1, p.name, p2.name, p3.name
FROM (
    SELECT e.before_id AS id1, e2.before_id AS id2, e2.after_id AS id3
    FROM Evolution e, Evolution e2
    WHERE (e.before_id NOT IN (SELECT et.after_id FROM Evolution et) AND e.after_id = e2.before_id) AND e2.after_id NOT IN (SELECT et2.before_id FROM Evolution et2)
) AS TripleEvolution, Pokemon p, Pokemon p2, Pokemon p3
WHERE id1 = p.id AND id2 = p2.id AND id3 = p3.id
ORDER BY id1;