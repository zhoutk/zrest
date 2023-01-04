/* eslint-disable */
const assert = require('assert')
const { spec } = require('pactum')
const { like, eachLike } = require('pactum-matchers')

it('test /rs/db_init', async () => {
  await spec()
    .post('/rs/db_init')
    .expectJsonMatch({"status": 200})
});

it('should get a response with table not exist', async () => {
  await spec()
    .get('/rs/table_name_not_exist_in_db')
    .expectJsonMatch({"status": 701})
});

it('test /rs/table/id', async () => {
  const response = await spec()
    .get('/rs/table_for_test/a1b2c3d4')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['name'], 'Kevin 凯文')
  assert.equal(dataZero['age'], 18)
  assert.equal(dataZero['score'], 99.99)
});

it('test /rs/table?id&age', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('id', 'a1b2c3d4')
    .withQueryParams('age', 18)
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['score'], 99.99)
});

it('test /rs/table?id&age&name exact match not find', async () => {
  await spec()
    .get('/rs/table_for_test')
    .withQueryParams('id', 'a1b2c3d4')
    .withQueryParams('age', 18)
    .withQueryParams('name', 'Kevin')
    .expectJsonMatch({"status": 202})
});

it('test /rs/table?id&age&name exact match find', async () => {
  await spec()
    .get('/rs/table_for_test')
    .withQueryParams('id', 'a1b2c3d4')
    .withQueryParams('age', 18)
    .withQueryParams('name', 'Kevin 凯文')
    .expectJsonMatch({"status": 200})
});

it('test /rs/table?id&age&name fuzzy match find', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('name', 'Kevin')
    .withQueryParams('fuzzy', 1)
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['name'], 'Kevin 凯文')
});

it('test update data', async () => {
  await spec()
    .put('/rs/table_for_test')
    .withBody({
      id: 'a1b2c3d4',
      score: 6.6
    })
    .expectJsonMatch({"status": 200})
});

it('test update data is right or not', async () => {
  const response = await spec()
    .get('/rs/table_for_test/a1b2c3d4')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['score'], 6.6)
});

it('test remove data', async () => {
  await spec()
    .delete('/rs/table_for_test')
    .withQueryParams('id', 'a1b2c3d4')
    .expectJsonMatch({"status": 200})
});

it('test remove data is right or not', async () => {
  await spec()
    .get('/rs/table_for_test/a1b2c3d4')
    .expectJsonMatch({"status": 202})
});

it('test fuzzy match return multi records', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('name', 'test')
    .withQueryParams('fuzzy', 1)
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data']
  assert.equal(dataZero.length, 5)
});

it('test update data multi values', async () => {
  await spec()
    .put('/rs/table_for_test')
    .withBody({
      id: 'a5b6c7d8',
      name: 'test888',
      score: 23.27,
      age: 22
    })
    .expectJsonMatch({"status": 200})
});

it('test retrieve by id', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('id', 'a5b6c7d8')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['name'], 'test888')
  assert.equal(dataZero['score'], 23.27)
  assert.equal(dataZero['age'], 22)
});