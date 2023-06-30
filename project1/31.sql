SELECT p.type
FROM Pokemon p
WHERE p.id IN (SELECT before_id FROM Evolution)
GROUP BY p.type
HAVING COUNT(p.id) >= 3
ORDER BY p.type DESC;