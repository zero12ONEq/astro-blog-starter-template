import { getCollection } from 'astro:content';

export async function getPosts() {
	return (await getCollection('blog')).sort(
		(a, b) =>
			a.data.order - b.data.order || b.data.pubDate.valueOf() - a.data.pubDate.valueOf(),
	);
}
