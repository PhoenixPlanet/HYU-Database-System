SELECT p.type, COUNT(p.type) AS type_count
FROM Pokemon p
GROUP BY p.type
ORDER BY type_count, p.type;