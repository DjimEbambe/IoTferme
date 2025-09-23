import { expect } from 'chai';
import { computeUserRooms } from '../src/sockets/rooms.js';

describe('Socket rooms', () => {
  it('derives rooms from scopes', () => {
    const rooms = computeUserRooms({
      scopes: {
        site: ['SITE:KIN-GOLIATH'],
        building: ['BLDG:A'],
        zone: ['A-Z1'],
      },
    });
    expect(rooms).to.include('site:SITE:KIN-GOLIATH');
    expect(rooms).to.include('building:BLDG:A');
    expect(rooms).to.include('zone:A-Z1');
    expect(rooms).to.include('dashboard');
  });
});
