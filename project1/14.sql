SELECT p.name
FROM Pokemon p, Evolution e
WHERE p.type = 'Grass' AND p.id = e.before_id;