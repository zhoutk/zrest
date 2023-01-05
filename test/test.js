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

it('test query using ins key', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('ins', 'age,20,21,23')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data']
  assert.equal(dataZero.length, 3)
});

it('test query using lks key', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('lks', 'name,001,age,23')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data']
  assert.equal(dataZero.length, 2)
});

it('test query using ors key', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('ors', 'age,19,age,23')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data']
  assert.equal(dataZero.length, 2)
});

it('test query using page and size', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('page', 1)
    .withQueryParams('size', 3)
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data']
  assert.equal(dataZero.length, 3)
});

it('test query using count key', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('count', '1,total')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['total'], 5)
});

it('test query using sum key', async () => {
  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('sum', 'age,agesum')
    .withQueryParams('age', '<=,20')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['agesum'], 39)
});

it('test query using group key', async () => {
  await spec()
    .put('/rs/table_for_test')
    .withBody({
      id: 'a4b5c6d7',
      age: 22
    })
    .expectJsonMatch({"status": 200})

  const response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('group', 'age')
    .withQueryParams('count', '*,total')
    .withQueryParams('sort', 'total desc')
    .expectJsonMatch({"status": 200})

  const dataZero = response.json['data'][0]
  assert.equal(dataZero['total'], 2)
});

it('test query using greater than and less than', async () => {
  let response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('age', '>,21')
    .expectJsonMatch({"status": 200})

  let dataZero = response.json['data']
  assert.equal(dataZero.length, 3)

  response = await spec()
    .get('/rs/table_for_test')
    .withQueryParams('age', '>=,19,<=,22')
    .expectJsonMatch({"status": 200})

  dataZero = response.json['data']
  assert.equal(dataZero.length, 4)
});